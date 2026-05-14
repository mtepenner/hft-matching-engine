package main

import (
	"bufio"
	"bytes"
	"fmt"
	"io"
	"log"
	"net"
	"strconv"
	"strings"

	"github.com/mtepenner/hft-matching-engine/fix_gateway/internal/protocol"
	"github.com/mtepenner/hft-matching-engine/fix_gateway/internal/sequencer"
)

const listenAddr = ":9878"

func main() {
	sequencer.StartConsumer()

	ln, err := net.Listen("tcp", listenAddr)
	if err != nil {
		log.Fatalf("[FIXGateway] listen error: %v", err)
	}
	log.Printf("[FIXGateway] Listening for FIX connections on %s", listenAddr)

	for {
		conn, err := ln.Accept()
		if err != nil {
			log.Printf("[FIXGateway] accept error: %v", err)
			continue
		}
		go handleConn(conn)
	}
}

func handleConn(conn net.Conn) {
	defer conn.Close()
	remote := conn.RemoteAddr().String()
	log.Printf("[FIXGateway] client connected: %s", remote)

	reader := bufio.NewReader(conn)
	for {
		// Read until SOH-terminated message (simplified framing)
		rawMsg, err := reader.ReadBytes('\x01')
		if err != nil {
			if err != io.EOF {
				log.Printf("[FIXGateway] read error from %s: %v", remote, err)
			}
			return
		}

		msg, err := protocol.Parse(rawMsg)
		if err != nil {
			log.Printf("[FIXGateway] parse error: %v", err)
			continue
		}

		switch msg.MsgType() {
		case "D": // NewOrderSingle
			handleNewOrder(msg, conn)
		case "F": // OrderCancelRequest
			sendExecutionReport(conn, msg.Tag(11), "4", "6") // cancelled
		case "A": // Logon
			sendLogonAck(conn)
		default:
			log.Printf("[FIXGateway] unhandled MsgType=%s", msg.MsgType())
		}
	}
}

func handleNewOrder(msg *protocol.FIXMessage, conn net.Conn) {
	symbol := msg.Tag(55)
	sideStr := msg.Tag(54)
	priceStr := msg.Tag(44)
	qtyStr := msg.Tag(38)
	clOrdID := msg.Tag(11)

	side := byte('B')
	if sideStr == "2" {
		side = 'S'
	}

	priceF, _ := strconv.ParseFloat(priceStr, 64)
	price := int64(priceF * 100) // convert to ticks
	qty, _ := strconv.ParseUint(qtyStr, 10, 32)

	var sym [8]byte
	copy(sym[:], symbol)

	ev := sequencer.OrderEvent{
		ClientID:  1,
		Symbol:    sym,
		Side:      side,
		Price:     price,
		Qty:       uint32(qty),
		OrderType: 'L',
	}

	seq, err := sequencer.Publish(ev)
	if err != nil {
		sendExecutionReport(conn, clOrdID, "8", "1") // rejected
		return
	}

	log.Printf("[FIXGateway] NewOrder sym=%s side=%c price=%d qty=%d seq=%d",
		symbol, side, price, qty, seq)
	sendExecutionReport(conn, clOrdID, "0", "1") // new/accepted
}

func sendExecutionReport(conn net.Conn, clOrdID, execType, ordStatus string) {
	tags := map[int]string{
		8:   "FIX.4.2",
		35:  "8", // ExecutionReport
		11:  clOrdID,
		17:  fmt.Sprintf("ER%s", clOrdID),
		20:  "0",
		39:  ordStatus,
		150: execType,
		14:  "0",
		151: "0",
	}
	conn.Write(protocol.Encode(tags))
}

func sendLogonAck(conn net.Conn) {
	tags := map[int]string{
		8:   "FIX.4.2",
		35:  "A",
		98:  "0",
		108: "30",
	}
	conn.Write(protocol.Encode(tags))
}

var _ = strings.TrimSpace
var _ = bytes.Compare

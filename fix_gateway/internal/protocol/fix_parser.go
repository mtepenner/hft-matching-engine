// Package protocol implements a binary-fast FIX tag-value parser.
package protocol

import (
	"bytes"
	"fmt"
	"strconv"
)

// FIXMessage holds parsed FIX tag/value pairs.
type FIXMessage struct {
	Tags map[int]string
	Raw  []byte
}

const SOH = byte(1) // FIX field delimiter

// Parse parses a raw FIX message byte slice.
func Parse(raw []byte) (*FIXMessage, error) {
	msg := &FIXMessage{
		Tags: make(map[int]string, 32),
		Raw:  raw,
	}

	fields := bytes.Split(raw, []byte{SOH})
	for _, field := range fields {
		if len(field) == 0 {
			continue
		}
		eq := bytes.IndexByte(field, '=')
		if eq < 0 {
			continue
		}
		tagStr := string(field[:eq])
		val := string(field[eq+1:])
		tag, err := strconv.Atoi(tagStr)
		if err != nil {
			continue // skip malformed
		}
		msg.Tags[tag] = val
	}

	// Validate required fields: BeginString(8), MsgType(35), MsgSeqNum(34)
	if _, ok := msg.Tags[8]; !ok {
		return nil, fmt.Errorf("missing BeginString (tag 8)")
	}
	if _, ok := msg.Tags[35]; !ok {
		return nil, fmt.Errorf("missing MsgType (tag 35)")
	}

	return msg, nil
}

// Encode serialises a FIXMessage back to wire format.
func Encode(tags map[int]string) []byte {
	var buf bytes.Buffer
	// Required ordering per FIX spec: 8, 9, 35, then the rest, then 10
	ordered := []int{8, 9, 35}
	written := map[int]bool{8: false, 9: false, 35: false, 10: false}

	for _, t := range ordered {
		if v, ok := tags[t]; ok {
			buf.WriteString(strconv.Itoa(t))
			buf.WriteByte('=')
			buf.WriteString(v)
			buf.WriteByte(SOH)
			written[t] = true
		}
	}
	for t, v := range tags {
		if !written[t] && t != 10 {
			buf.WriteString(strconv.Itoa(t))
			buf.WriteByte('=')
			buf.WriteString(v)
			buf.WriteByte(SOH)
		}
	}
	// Checksum (tag 10): sum of all bytes mod 256
	checksum := 0
	for _, b := range buf.Bytes() {
		checksum += int(b)
	}
	buf.WriteString(fmt.Sprintf("10=%03d", checksum%256))
	buf.WriteByte(SOH)
	return buf.Bytes()
}

// MsgType returns the FIX MsgType (tag 35) value.
func (m *FIXMessage) MsgType() string { return m.Tags[35] }

// Tag is a convenience getter.
func (m *FIXMessage) Tag(n int) string { return m.Tags[n] }

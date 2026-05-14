import React, { useState } from 'react';

interface Props {
  onSubmit: (order: { symbol: string; side: 'buy' | 'sell'; price: number; qty: number }) => void;
}

const inputStyle: React.CSSProperties = {
  background: '#1a1a2e',
  border: '1px solid #333',
  color: '#e0e0e0',
  padding: '6px 10px',
  borderRadius: 4,
  width: '100%',
  marginBottom: 8,
};

export const OrderEntry: React.FC<Props> = ({ onSubmit }) => {
  const [symbol, setSymbol] = useState('AAPL');
  const [side, setSide] = useState<'buy' | 'sell'>('buy');
  const [price, setPrice] = useState('');
  const [qty, setQty] = useState('');

  const handleSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    onSubmit({ symbol, side, price: parseFloat(price), qty: parseInt(qty, 10) });
    setPrice('');
    setQty('');
  };

  return (
    <form onSubmit={handleSubmit} style={{ background: '#0d0d1a', padding: 16, borderRadius: 8 }}>
      <h3 style={{ color: '#e0e0e0', margin: '0 0 12px' }}>New Order</h3>

      <label style={{ color: '#aaa', fontSize: 12 }}>Symbol</label>
      <input style={inputStyle} value={symbol} onChange={e => setSymbol(e.target.value.toUpperCase())} />

      <label style={{ color: '#aaa', fontSize: 12 }}>Side</label>
      <div style={{ display: 'flex', gap: 8, marginBottom: 8 }}>
        {(['buy', 'sell'] as const).map(s => (
          <button
            key={s}
            type="button"
            onClick={() => setSide(s)}
            style={{
              flex: 1,
              padding: '6px 0',
              borderRadius: 4,
              border: 'none',
              cursor: 'pointer',
              background: side === s ? (s === 'buy' ? '#00c896' : '#ff4d4d') : '#2a2a3e',
              color: '#fff',
              fontWeight: side === s ? 700 : 400,
            }}
          >
            {s.toUpperCase()}
          </button>
        ))}
      </div>

      <label style={{ color: '#aaa', fontSize: 12 }}>Limit Price</label>
      <input
        style={inputStyle}
        type="number"
        step="0.01"
        placeholder="0.00"
        value={price}
        onChange={e => setPrice(e.target.value)}
        required
      />

      <label style={{ color: '#aaa', fontSize: 12 }}>Quantity</label>
      <input
        style={inputStyle}
        type="number"
        placeholder="100"
        value={qty}
        onChange={e => setQty(e.target.value)}
        required
      />

      <button
        type="submit"
        style={{
          width: '100%',
          padding: '8px 0',
          borderRadius: 4,
          border: 'none',
          background: side === 'buy' ? '#00c896' : '#ff4d4d',
          color: '#fff',
          fontWeight: 700,
          cursor: 'pointer',
          fontSize: 14,
        }}
      >
        Submit {side === 'buy' ? 'BUY' : 'SELL'}
      </button>
    </form>
  );
};

import React from 'react';

interface Tick {
  symbol: string;
  price: number;
  change: number;
  volume: number;
}

interface Props {
  ticks: Tick[];
}

export const TickerTape: React.FC<Props> = ({ ticks }) => (
  <div
    style={{
      display: 'flex',
      gap: 24,
      background: '#0d0d1a',
      padding: '8px 16px',
      overflowX: 'auto',
      borderBottom: '1px solid #222',
    }}
  >
    {ticks.map(t => (
      <div key={t.symbol} style={{ display: 'flex', flexDirection: 'column', minWidth: 80 }}>
        <span style={{ color: '#e0e0e0', fontWeight: 700, fontSize: 13 }}>{t.symbol}</span>
        <span style={{ color: '#fff', fontSize: 15 }}>{t.price.toFixed(2)}</span>
        <span style={{ color: t.change >= 0 ? '#00c896' : '#ff4d4d', fontSize: 11 }}>
          {t.change >= 0 ? '+' : ''}
          {t.change.toFixed(2)}%
        </span>
        <span style={{ color: '#888', fontSize: 10 }}>{(t.volume / 1000).toFixed(1)}K</span>
      </div>
    ))}
  </div>
);

import React, { useEffect, useRef } from 'react';
import {
  BarChart, Bar, XAxis, YAxis, Tooltip, ResponsiveContainer, Cell,
} from 'recharts';

interface Level {
  price: string;
  qty: string;
}

interface Props {
  bids: Level[];
  asks: Level[];
}

export const DepthChart: React.FC<Props> = ({ bids, asks }) => {
  const bidData = bids.map(b => ({ price: parseFloat(b.price), qty: parseFloat(b.qty), side: 'bid' }));
  const askData = asks.map(a => ({ price: parseFloat(a.price), qty: parseFloat(a.qty), side: 'ask' }));
  const data = [...bidData.reverse(), ...askData];

  return (
    <div style={{ background: '#0d0d1a', borderRadius: 8, padding: 12 }}>
      <h3 style={{ color: '#e0e0e0', margin: '0 0 8px' }}>Order Book Depth</h3>
      <ResponsiveContainer width="100%" height={220}>
        <BarChart data={data} margin={{ top: 4, right: 8, left: 8, bottom: 4 }}>
          <XAxis dataKey="price" tick={{ fill: '#aaa', fontSize: 10 }} />
          <YAxis tick={{ fill: '#aaa', fontSize: 10 }} />
          <Tooltip
            contentStyle={{ background: '#1a1a2e', border: '1px solid #333' }}
            labelStyle={{ color: '#e0e0e0' }}
          />
          <Bar dataKey="qty">
            {data.map((entry, i) => (
              <Cell key={i} fill={entry.side === 'bid' ? '#00c896' : '#ff4d4d'} />
            ))}
          </Bar>
        </BarChart>
      </ResponsiveContainer>
    </div>
  );
};

/**
 * FastTableRenderer – renders a large list of rows using a virtual table backed by
 * an offscreen canvas buffer for maximum throughput.  When the data changes faster
 * than the browser's 60 fps budget, frames are coalesced via requestAnimationFrame.
 */
export interface TableRow {
  cells: (string | number)[];
}

export class FastTableRenderer {
  private canvas: HTMLCanvasElement;
  private ctx: CanvasRenderingContext2D;
  private rows: TableRow[] = [];
  private rafId = 0;
  private dirty = false;

  readonly rowHeight = 20;
  readonly font = '12px monospace';
  readonly headerColor = '#1a1a2e';
  readonly bidColor = '#00c896';
  readonly askColor = '#ff4d4d';
  readonly textColor = '#e0e0e0';
  readonly bgColor = '#0d0d1a';

  constructor(canvas: HTMLCanvasElement) {
    this.canvas = canvas;
    this.ctx = canvas.getContext('2d')!;
  }

  setRows(rows: TableRow[]) {
    this.rows = rows;
    if (!this.dirty) {
      this.dirty = true;
      this.rafId = requestAnimationFrame(() => this.flush());
    }
  }

  private flush() {
    this.dirty = false;
    const { canvas, ctx, rows, rowHeight, font, textColor, bgColor, bidColor, askColor } = this;

    ctx.fillStyle = bgColor;
    ctx.fillRect(0, 0, canvas.width, canvas.height);
    ctx.font = font;

    rows.forEach((row, i) => {
      const y = i * rowHeight;
      const isBid = row.cells[2] === 'BID';
      ctx.fillStyle = isBid ? bidColor : askColor;
      ctx.fillRect(0, y, 6, rowHeight);

      ctx.fillStyle = textColor;
      row.cells.forEach((cell, j) => {
        ctx.fillText(String(cell), 12 + j * 100, y + rowHeight - 5);
      });
    });
  }

  destroy() {
    cancelAnimationFrame(this.rafId);
  }
}

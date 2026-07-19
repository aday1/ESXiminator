using System.Drawing;
using System.Drawing.Drawing2D;
using System.Windows.Forms;

namespace ESXiminator.Hub
{
    internal static class Theme
    {
        public static readonly Color Body = Color.FromArgb(0x8f, 0x1a, 0x12);
        public static readonly Color BodyDark = Color.FromArgb(0x5c, 0x0e, 0x09);
        public static readonly Color Panel = Color.FromArgb(0x23, 0x20, 0x22);
        public static readonly Color PanelHi = Color.FromArgb(0x37, 0x32, 0x2f);
        public static readonly Color MetalTop = Color.FromArgb(0xd8, 0xd4, 0xcd);
        public static readonly Color MetalBot = Color.FromArgb(0x8d, 0x88, 0x80);
        public static readonly Color Text = Color.FromArgb(0xe8, 0xe2, 0xd8);
        public static readonly Color Amber = Color.FromArgb(0xff, 0xb1, 0x4a);
        public static readonly Color Led = Color.FromArgb(0xff, 0x55, 0x33);
        public static readonly Color Muted = Color.FromArgb(0x9d, 0x96, 0x8a);

        public static Font TitleFont => new Font("Segoe UI", 18f, FontStyle.Bold | FontStyle.Italic);
        public static Font UiFont => new Font("Segoe UI", 9.5f, FontStyle.Regular);
        public static Font BoldFont => new Font("Segoe UI", 9.5f, FontStyle.Bold);

        public static void StylePrimaryButton(Button b)
        {
            b.FlatStyle = FlatStyle.Flat;
            b.FlatAppearance.BorderColor = Color.FromArgb(0x40, 0, 0, 0);
            b.FlatAppearance.BorderSize = 1;
            b.BackColor = Body;
            b.ForeColor = Color.White;
            b.Font = BoldFont;
            b.Cursor = Cursors.Hand;
            b.Height = 40;
        }

        public static void StyleSecondaryButton(Button b)
        {
            b.FlatStyle = FlatStyle.Flat;
            b.FlatAppearance.BorderColor = Color.FromArgb(0x60, 0, 0, 0);
            b.FlatAppearance.BorderSize = 1;
            b.BackColor = PanelHi;
            b.ForeColor = Text;
            b.Font = BoldFont;
            b.Cursor = Cursors.Hand;
            b.Height = 36;
        }

        public static void PaintMetalHeader(Graphics g, Rectangle bounds, string title, string subtitle)
        {
            using (var brush = new LinearGradientBrush(bounds, MetalTop, MetalBot, LinearGradientMode.Vertical))
                g.FillRectangle(brush, bounds);
            using (var pen = new Pen(Color.FromArgb(0xaa, 0, 0, 0), 3))
                g.DrawLine(pen, bounds.Left, bounds.Bottom - 1, bounds.Right, bounds.Bottom - 1);
            TextRenderer.DrawText(g, title, TitleFont, new Point(18, 10), BodyDark);
            TextRenderer.DrawText(g, subtitle, BoldFont, new Point(18, 42), Color.FromArgb(0x3a, 0x36, 0x32));
        }
    }
}

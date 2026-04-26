// ============================================================
//  ChessGUI.java  –  BeginWithChess  (Redesigned)
//
//  A modern, coaching-oriented chess GUI for beginners.
//  Neutral warm color palette (cream, stone, sage).
//
//  NEW FEATURES:
//    - Coach Panel with AI move explanations & blunder feedback
//    - Undo / Takeback button
//    - Evaluation bar showing who is winning
//    - Hover highlights, radial check glow, rounded UI
// ============================================================

import javax.swing.*;
import javax.swing.border.*;
import java.awt.*;
import java.awt.event.*;
import java.awt.geom.*;
import java.io.*;
import java.util.*;
import java.util.List;
import java.util.concurrent.*;

public class ChessGUI extends JFrame {

    private static final int BOARD_SIZE = 560;
    private static final int CELL_SIZE  = BOARD_SIZE / 8;
    private static final int EVAL_BAR_W = 28;
    private static final int SIDE_WIDTH = 300;

    // ── Color Palette (neutral warm) ─────────────────────────
    private static final Color SQ_LIGHT    = new Color(235,224,208);
    private static final Color SQ_DARK     = new Color(180,162,140);
    private static final Color HL_SELECT   = new Color(168,195,160,180);
    private static final Color HL_LEGAL    = new Color(140,175,130,160);
    private static final Color HL_LASTMOVE = new Color(210,195,130,130);
    private static final Color BG_MAIN     = new Color(245,241,235);
    private static final Color BG_PANEL    = new Color(255,252,248);
    private static final Color BG_CARD     = new Color(248,245,240);
    private static final Color BG_INPUT    = new Color(240,236,230);
    private static final Color TEXT_PRI    = new Color(55,50,45);
    private static final Color TEXT_SEC    = new Color(120,112,105);
    private static final Color TEXT_MUT    = new Color(165,158,150);
    private static final Color ACCENT      = new Color(120,150,120);
    private static final Color DANGER      = new Color(190,100,90);
    private static final Color WARNING     = new Color(205,165,80);
    private static final Color INFO_BG     = new Color(235,240,235);
    private static final Color BORDER_L    = new Color(220,215,208);
    private static final Color BORDER_M    = new Color(200,194,186);
    private static final Color EVAL_W      = new Color(250,248,244);
    private static final Color EVAL_B      = new Color(60,55,50);

    private static final Map<Integer,String> PIECE_U = new HashMap<>();
    static {
        PIECE_U.put(6,"\u2654"); PIECE_U.put(5,"\u2655"); PIECE_U.put(4,"\u2656");
        PIECE_U.put(3,"\u2657"); PIECE_U.put(2,"\u2658"); PIECE_U.put(1,"\u2659");
        PIECE_U.put(-6,"\u265A"); PIECE_U.put(-5,"\u265B"); PIECE_U.put(-4,"\u265C");
        PIECE_U.put(-3,"\u265D"); PIECE_U.put(-2,"\u265E"); PIECE_U.put(-1,"\u265F");
    }

    // ── State ────────────────────────────────────────────────
    private int[][] board = new int[8][8];
    private boolean whiteToMove = true, boardFlipped = false;
    private int selRow = -1, selCol = -1, hoverRow = -1, hoverCol = -1;
    private List<int[]> legalMoves = new ArrayList<>();
    private int[] lastFrom = null, lastTo = null;
    private String gameStatus = "NORMAL";
    private boolean aiThinking = false, playerIsWhite = true;
    private int aiDepth = 3, currentEval = 0, moveNumber = 1;
    private boolean whiteMoved = false;

    // ── Components ───────────────────────────────────────────
    private BoardPanel boardPanel;
    private EvalBar evalBar;
    private JTextPane coachPane;
    private JTextArea histArea;
    private JLabel statusLabel, turnDot;
    private JComboBox<String> diffBox;
    private EngineHandler engine;

    public static void main(String[] args) {
        System.setProperty("awt.useSystemAAFontSettings","on");
        System.setProperty("swing.aatext","true");
        SwingUtilities.invokeLater(() -> {
            try { UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName()); } catch(Exception e){}
            new ChessGUI().setVisible(true);
        });
    }

    public ChessGUI() {
        setTitle("BeginWithChess");
        setDefaultCloseOperation(EXIT_ON_CLOSE);
        setResizable(false);
        initBoard(); buildUI(); pack(); setLocationRelativeTo(null);
        engine = new EngineHandler(); engine.start();
    }

    private void initBoard() {
        int[] br = {4,2,3,5,6,3,2,4};
        for (int c=0;c<8;c++) { board[0][c]=-br[c]; board[1][c]=-1; board[6][c]=1; board[7][c]=br[c]; }
        for (int r=2;r<=5;r++) for (int c=0;c<8;c++) board[r][c]=0;
        whiteToMove=true; gameStatus="NORMAL"; currentEval=0;
    }

    // ══════════════════════════════════════════════════════════
    //  BUILD UI
    // ══════════════════════════════════════════════════════════
    private void buildUI() {
        JPanel root = new JPanel(new BorderLayout());
        root.setBackground(BG_MAIN); root.setBorder(new EmptyBorder(16,16,16,16));

        JPanel bc = new JPanel(new BorderLayout(6,0)); bc.setOpaque(false);
        evalBar = new EvalBar(); evalBar.setPreferredSize(new Dimension(EVAL_BAR_W, BOARD_SIZE));
        boardPanel = new BoardPanel(); boardPanel.setPreferredSize(new Dimension(BOARD_SIZE, BOARD_SIZE));
        bc.add(evalBar, BorderLayout.WEST); bc.add(boardPanel, BorderLayout.CENTER);

        JPanel sp = buildSidePanel(); sp.setPreferredSize(new Dimension(SIDE_WIDTH, BOARD_SIZE));
        JPanel rw = new JPanel(new BorderLayout()); rw.setOpaque(false);
        rw.add(Box.createHorizontalStrut(12), BorderLayout.WEST); rw.add(sp, BorderLayout.CENTER);

        root.add(bc, BorderLayout.CENTER); root.add(rw, BorderLayout.EAST);
        setContentPane(root);
    }

    private JPanel buildSidePanel() {
        JPanel p = new JPanel(); p.setLayout(new BoxLayout(p, BoxLayout.Y_AXIS));
        p.setBackground(BG_PANEL);
        p.setBorder(BorderFactory.createCompoundBorder(new RoundBdr(BORDER_L,12), new EmptyBorder(14,14,14,14)));

        JLabel logo = lbl("BeginWithChess", new Font("SansSerif",Font.BOLD,20), TEXT_PRI);
        JLabel tag  = lbl("Your chess coaching companion", new Font("SansSerif",Font.ITALIC,11), TEXT_MUT);

        JPanel sr = new JPanel(new FlowLayout(FlowLayout.LEFT,6,0)); sr.setOpaque(false);
        sr.setAlignmentX(LEFT_ALIGNMENT); sr.setMaximumSize(new Dimension(SIDE_WIDTH,28));
        turnDot = new JLabel("\u25CF"); turnDot.setFont(new Font("SansSerif",Font.PLAIN,14)); turnDot.setForeground(ACCENT);
        statusLabel = new JLabel("White to move"); statusLabel.setFont(new Font("SansSerif",Font.BOLD,13)); statusLabel.setForeground(TEXT_PRI);
        sr.add(turnDot); sr.add(statusLabel);

        JLabel ct = lbl("Coach", new Font("SansSerif",Font.BOLD,13), ACCENT);
        coachPane = new JTextPane(); coachPane.setEditable(false); coachPane.setBackground(INFO_BG);
        coachPane.setFont(new Font("SansSerif",Font.PLAIN,12)); coachPane.setForeground(TEXT_PRI);
        coachPane.setBorder(new EmptyBorder(10,12,10,12));
        coachPane.setText("Welcome! Click a piece to see where it can move.\n\nTip: Control the center early and develop your pieces!");
        JScrollPane cs = wrap(coachPane, 140);

        JLabel dl = lbl("Difficulty", new Font("SansSerif",Font.BOLD,12), TEXT_SEC);
        diffBox = new JComboBox<>(new String[]{"Beginner","Easy","Medium","Hard","Expert"});
        diffBox.setSelectedIndex(2); diffBox.setMaximumSize(new Dimension(SIDE_WIDTH-28,30));
        diffBox.setAlignmentX(LEFT_ALIGNMENT); diffBox.setBackground(BG_INPUT); diffBox.setForeground(TEXT_PRI);
        diffBox.setFont(new Font("SansSerif",Font.PLAIN,12));
        diffBox.addActionListener(e -> aiDepth = diffBox.getSelectedIndex()+1);

        JLabel pl = lbl("Play as", new Font("SansSerif",Font.BOLD,12), TEXT_SEC);
        JPanel cp = new JPanel(new GridLayout(1,2,8,0)); cp.setOpaque(false);
        cp.setAlignmentX(LEFT_ALIGNMENT); cp.setMaximumSize(new Dimension(SIDE_WIDTH-28,34));
        JButton wb = pill("\u2654 White", new Color(230,240,230), TEXT_PRI);
        JButton bb = pill("\u265A Black", new Color(65,60,55), new Color(230,225,218));
        wb.addActionListener(e -> { playerIsWhite=true; newGame(); });
        bb.addActionListener(e -> { playerIsWhite=false; newGame(); });
        cp.add(wb); cp.add(bb);

        JLabel hl = lbl("Moves", new Font("SansSerif",Font.BOLD,12), TEXT_SEC);
        histArea = new JTextArea(6,20); histArea.setEditable(false); histArea.setBackground(BG_CARD);
        histArea.setForeground(TEXT_SEC); histArea.setFont(new Font("Monospaced",Font.PLAIN,11));
        histArea.setBorder(new EmptyBorder(8,10,8,10));
        JScrollPane hs = wrap(histArea, 120);

        JPanel br = new JPanel(new GridLayout(1,3,8,0)); br.setOpaque(false);
        br.setAlignmentX(LEFT_ALIGNMENT); br.setMaximumSize(new Dimension(SIDE_WIDTH-28,36));
        JButton nb = acBtn("New Game", ACCENT), ub = acBtn("Undo", WARNING), fb = acBtn("Flip", TEXT_SEC);
        nb.addActionListener(e -> newGame()); ub.addActionListener(e -> undoMove());
        fb.addActionListener(e -> { boardFlipped=!boardFlipped; boardPanel.repaint(); evalBar.repaint(); });
        br.add(nb); br.add(ub); br.add(fb);

        p.add(logo);  p.add(Box.createVerticalStrut(2));  p.add(tag);  p.add(Box.createVerticalStrut(12));
        p.add(sr);     p.add(Box.createVerticalStrut(14)); p.add(ct);   p.add(Box.createVerticalStrut(6));
        p.add(cs);     p.add(Box.createVerticalStrut(14)); p.add(dl);   p.add(Box.createVerticalStrut(4));
        p.add(diffBox);p.add(Box.createVerticalStrut(10)); p.add(pl);   p.add(Box.createVerticalStrut(4));
        p.add(cp);     p.add(Box.createVerticalStrut(14)); p.add(hl);   p.add(Box.createVerticalStrut(4));
        p.add(hs);     p.add(Box.createVerticalStrut(14)); p.add(br);
        return p;
    }

    private JLabel lbl(String t, Font f, Color c) {
        JLabel l = new JLabel(t); l.setFont(f); l.setForeground(c); l.setAlignmentX(LEFT_ALIGNMENT); return l;
    }
    private JScrollPane wrap(JComponent c, int h) {
        JScrollPane s = new JScrollPane(c); s.setAlignmentX(LEFT_ALIGNMENT);
        s.setMaximumSize(new Dimension(SIDE_WIDTH-28,h)); s.setPreferredSize(new Dimension(SIDE_WIDTH-28,h));
        s.setBorder(new RoundBdr(BORDER_L,8)); s.getVerticalScrollBar().setUnitIncrement(8); return s;
    }
    private JButton pill(String t, final Color bg, Color fg) {
        JButton b = new JButton(t) {
            @Override protected void paintComponent(Graphics g) {
                Graphics2D g2=(Graphics2D)g.create();
                g2.setRenderingHint(RenderingHints.KEY_ANTIALIASING,RenderingHints.VALUE_ANTIALIAS_ON);
                g2.setColor(getModel().isRollover()?bg.brighter():bg);
                g2.fillRoundRect(0,0,getWidth(),getHeight(),10,10); g2.dispose();
                super.paintComponent(g);
            }
        };
        b.setFont(new Font("SansSerif",Font.BOLD,12)); b.setForeground(fg);
        b.setFocusPainted(false); b.setBorderPainted(false); b.setContentAreaFilled(false); b.setOpaque(false);
        b.setCursor(Cursor.getPredefinedCursor(Cursor.HAND_CURSOR)); b.setRolloverEnabled(true); return b;
    }
    private JButton acBtn(String t, final Color bg) {
        JButton b = new JButton(t) {
            boolean hov=false;
            { addMouseListener(new MouseAdapter(){ public void mouseEntered(MouseEvent e){hov=true;repaint();} public void mouseExited(MouseEvent e){hov=false;repaint();} }); }
            @Override protected void paintComponent(Graphics g) {
                Graphics2D g2=(Graphics2D)g.create();
                g2.setRenderingHint(RenderingHints.KEY_ANTIALIASING,RenderingHints.VALUE_ANTIALIAS_ON);
                g2.setColor(hov?bg.darker():bg); g2.fillRoundRect(0,0,getWidth(),getHeight(),10,10); g2.dispose();
                super.paintComponent(g);
            }
        };
        b.setFont(new Font("SansSerif",Font.BOLD,11)); b.setForeground(Color.WHITE);
        b.setFocusPainted(false); b.setBorderPainted(false); b.setContentAreaFilled(false); b.setOpaque(false);
        b.setCursor(Cursor.getPredefinedCursor(Cursor.HAND_CURSOR)); return b;
    }

    // ══════════════════════════════════════════════════════════
    //  EVAL BAR
    // ══════════════════════════════════════════════════════════
    class EvalBar extends JPanel {
        EvalBar() { setBackground(EVAL_B); setToolTipText("Evaluation bar"); }
        @Override protected void paintComponent(Graphics g) {
            super.paintComponent(g);
            Graphics2D g2=(Graphics2D)g.create();
            g2.setRenderingHint(RenderingHints.KEY_ANTIALIASING,RenderingHints.VALUE_ANTIALIAS_ON);
            int w=getWidth(), h=getHeight();
            double ev=Math.max(-1000,Math.min(1000,currentEval));
            double wp=0.5+(ev/2000.0); if (boardFlipped) wp=1.0-wp;
            int wh=(int)(h*wp);
            g2.setColor(EVAL_B); g2.fillRoundRect(0,0,w,h,8,8);
            g2.setColor(EVAL_W); g2.fillRect(0,h-wh,w,wh);
            g2.setColor(BORDER_M); g2.fillRect(0,h/2-1,w,1);
            g2.setFont(new Font("SansSerif",Font.BOLD,9));
            double ae=Math.abs(currentEval/100.0);
            String et=currentEval>0?String.format("+%.1f",ae):currentEval<0?String.format("-%.1f",ae):"0.0";
            FontMetrics fm=g2.getFontMetrics(); int tw=fm.stringWidth(et);
            int ty=h-wh; if(ty<14) ty=14; if(ty>h-4) ty=h-4;
            g2.setColor(wh>h/2?TEXT_PRI:EVAL_W); g2.drawString(et,(w-tw)/2,ty);
            g2.setColor(BORDER_M); g2.drawRoundRect(0,0,w-1,h-1,8,8); g2.dispose();
        }
    }

    // ══════════════════════════════════════════════════════════
    //  BOARD PANEL
    // ══════════════════════════════════════════════════════════
    class BoardPanel extends JPanel {
        BoardPanel() {
            setBackground(BG_MAIN); setCursor(Cursor.getPredefinedCursor(Cursor.HAND_CURSOR));
            addMouseListener(new MouseAdapter() {
                public void mouseClicked(MouseEvent e) { handleClick(e.getX(),e.getY()); }
                public void mouseExited(MouseEvent e)  { hoverRow=-1; hoverCol=-1; repaint(); }
            });
            addMouseMotionListener(new MouseMotionAdapter() {
                public void mouseMoved(MouseEvent e) {
                    int[] p=stb(e.getX(),e.getY());
                    if(p[0]!=hoverRow||p[1]!=hoverCol){hoverRow=p[0];hoverCol=p[1];repaint();}
                }
            });
        }

        @Override protected void paintComponent(Graphics g) {
            super.paintComponent(g);
            Graphics2D g2=(Graphics2D)g;
            g2.setRenderingHint(RenderingHints.KEY_ANTIALIASING,RenderingHints.VALUE_ANTIALIAS_ON);
            g2.setRenderingHint(RenderingHints.KEY_TEXT_ANTIALIASING,RenderingHints.VALUE_TEXT_ANTIALIAS_ON);
            g2.setRenderingHint(RenderingHints.KEY_RENDERING,RenderingHints.VALUE_RENDER_QUALITY);
            drawSquares(g2); drawCoords(g2); drawLast(g2); drawHover(g2);
            drawSel(g2); drawCheck(g2); drawPieces(g2); drawDots(g2);
            g2.setColor(BORDER_M); g2.setStroke(new BasicStroke(2));
            g2.drawRoundRect(0,0,BOARD_SIZE-1,BOARD_SIZE-1,6,6);
        }

        private void drawSquares(Graphics2D g) {
            Shape old=g.getClip();
            g.setClip(new RoundRectangle2D.Double(0,0,BOARD_SIZE,BOARD_SIZE,6,6));
            for(int r=0;r<8;r++) for(int c=0;c<8;c++) {
                int dr=boardFlipped?7-r:r, dc=boardFlipped?7-c:c;
                g.setColor(((r+c)%2==0)?SQ_LIGHT:SQ_DARK);
                g.fillRect(dc*CELL_SIZE,dr*CELL_SIZE,CELL_SIZE,CELL_SIZE);
            }
            g.setClip(old);
        }

        private void drawCoords(Graphics2D g) {
            g.setFont(new Font("SansSerif",Font.BOLD,9));
            for(int i=0;i<8;i++) {
                int rk=boardFlipped?(i+1):(8-i); char fl=(char)('a'+(boardFlipped?(7-i):i));
                g.setColor(((i%2)==0)?new Color(155,140,122):new Color(190,178,162));
                g.drawString(String.valueOf(rk),3,i*CELL_SIZE+12);
                g.setColor(((i%2)==0)?new Color(190,178,162):new Color(155,140,122));
                g.drawString(String.valueOf(fl),i*CELL_SIZE+CELL_SIZE-10,BOARD_SIZE-3);
            }
        }

        private void drawLast(Graphics2D g) {
            if(lastFrom==null) return; g.setColor(HL_LASTMOVE);
            int[] f=bts(lastFrom[0],lastFrom[1]), t=bts(lastTo[0],lastTo[1]);
            g.fillRect(f[0],f[1],CELL_SIZE,CELL_SIZE); g.fillRect(t[0],t[1],CELL_SIZE,CELL_SIZE);
        }

        private void drawHover(Graphics2D g) {
            if(hoverRow<0||hoverRow>7||hoverCol<0||hoverCol>7||selRow>=0||aiThinking||!isPlayerTurn()) return;
            int pc=board[hoverRow][hoverCol];
            if(pc!=0&&(playerIsWhite?pc>0:pc<0)) {
                int[] pos=bts(hoverRow,hoverCol);
                g.setColor(new Color(168,195,160,60));
                g.fillRect(pos[0],pos[1],CELL_SIZE,CELL_SIZE);
            }
        }

        private void drawSel(Graphics2D g) {
            if(selRow<0) return; g.setColor(HL_SELECT);
            int[] pos=bts(selRow,selCol); g.fillRect(pos[0],pos[1],CELL_SIZE,CELL_SIZE);
        }

        private void drawCheck(Graphics2D g) {
            if(!gameStatus.contains("CHECK")&&!gameStatus.equals("CHECKMATE")) return;
            int king=whiteToMove?6:-6;
            for(int r=0;r<8;r++) for(int c=0;c<8;c++) if(board[r][c]==king) {
                int[] pos=bts(r,c); int cx=pos[0]+CELL_SIZE/2, cy=pos[1]+CELL_SIZE/2;
                g.setPaint(new RadialGradientPaint(cx,cy,CELL_SIZE*0.55f,
                    new float[]{0f,0.6f,1f},
                    new Color[]{new Color(220,80,70,200),new Color(220,80,70,100),new Color(220,80,70,0)}));
                g.fillRect(pos[0],pos[1],CELL_SIZE,CELL_SIZE);
            }
        }

        private void drawDots(Graphics2D g) {
            for(int[] t:legalMoves) {
                int[] pos=bts(t[0],t[1]); int cx=pos[0]+CELL_SIZE/2, cy=pos[1]+CELL_SIZE/2;
                if(board[t[0]][t[1]]!=0) {
                    g.setColor(new Color(180,90,80,130)); g.setStroke(new BasicStroke(3.5f));
                    g.drawRoundRect(pos[0]+3,pos[1]+3,CELL_SIZE-6,CELL_SIZE-6,4,4);
                    g.setStroke(new BasicStroke(1));
                } else { g.setColor(HL_LEGAL); int r=CELL_SIZE/7; g.fillOval(cx-r,cy-r,r*2,r*2); }
            }
        }

        private void drawPieces(Graphics2D g) {
            String[] fns={"Segoe UI Symbol","Apple Color Emoji","DejaVu Sans","Noto Sans Symbols2","Arial Unicode MS"};
            Font pf=null;
            for(String fn:fns){Font f=new Font(fn,Font.PLAIN,CELL_SIZE-8);if(f.canDisplay('\u2654')){pf=f;break;}}
            if(pf==null) pf=new Font(Font.SANS_SERIF,Font.PLAIN,CELL_SIZE-8);
            g.setFont(pf);
            for(int r=0;r<8;r++) for(int c=0;c<8;c++) {
                int pc=board[r][c]; if(pc==0) continue;
                String sym=PIECE_U.get(pc); if(sym==null) continue;
                int[] pos=bts(r,c); FontMetrics fm=g.getFontMetrics();
                int x=pos[0]+(CELL_SIZE-fm.stringWidth(sym))/2, y=pos[1]+(CELL_SIZE+fm.getAscent())/2-4;
                g.setColor(new Color(0,0,0,35)); g.drawString(sym,x+1,y+2);
                g.setColor(pc>0?new Color(255,253,248):new Color(45,40,38)); g.drawString(sym,x,y);
                if(pc>0){g.setColor(new Color(130,122,110,80));g.drawString(sym,x+1,y);}
            }
        }

        private int[] bts(int r,int c) {
            int dr=boardFlipped?7-r:r, dc=boardFlipped?7-c:c;
            return new int[]{dc*CELL_SIZE,dr*CELL_SIZE};
        }
    }

    private int[] stb(int x,int y) {
        int dc=Math.max(0,Math.min(7,x/CELL_SIZE)), dr=Math.max(0,Math.min(7,y/CELL_SIZE));
        return new int[]{boardFlipped?7-dr:dr, boardFlipped?7-dc:dc};
    }

    // ══════════════════════════════════════════════════════════
    //  COACHING
    // ══════════════════════════════════════════════════════════
    private void showCoach(String fb, String ex, boolean ai) {
        StringBuilder sb=new StringBuilder();
        if(ex!=null&&!ex.isEmpty()) sb.append(ai?"\uD83E\uDD16 ":"\u265F ").append(ex);
        if(fb!=null&&!fb.isEmpty()) {
            if(sb.length()>0) sb.append("\n\n");
            if(fb.startsWith("BLUNDER"))       sb.append("\u26A0\uFE0F ");
            else if(fb.startsWith("Mistake"))   sb.append("\u26A0\uFE0F ");
            else if(fb.startsWith("Inaccuracy"))sb.append("\uD83D\uDCA1 ");
            else if(fb.contains("Excellent"))   sb.append("\u2B50 ");
            else if(fb.contains("Good"))        sb.append("\u2705 ");
            sb.append(fb);
        }
        if(sb.length()>0){coachPane.setText(sb.toString());coachPane.setCaretPosition(0);}
    }

    // ══════════════════════════════════════════════════════════
    //  GAME LOGIC
    // ══════════════════════════════════════════════════════════
    private void handleClick(int x, int y) {
        if(aiThinking||!isPlayerTurn()) return;
        if(gameStatus.equals("CHECKMATE")||gameStatus.equals("STALEMATE")) return;
        int[] pos=stb(x,y); int r=pos[0],c=pos[1];
        if(selRow>=0) {
            for(int[] t:legalMoves) if(t[0]==r&&t[1]==c)
                { execMove(selRow,selCol,r,c); selRow=selCol=-1; legalMoves.clear(); boardPanel.repaint(); return; }
            if(board[r][c]!=0&&(playerIsWhite?board[r][c]>0:board[r][c]<0)){selectPiece(r,c);return;}
            selRow=selCol=-1; legalMoves.clear(); boardPanel.repaint(); return;
        }
        if(board[r][c]!=0&&(playerIsWhite?board[r][c]>0:board[r][c]<0)) selectPiece(r,c);
    }

    private boolean isPlayerTurn(){return whiteToMove==playerIsWhite;}

    private void selectPiece(int r,int c) {
        selRow=r;selCol=c;legalMoves.clear();
        engine.send("LEGAL "+(char)('a'+c)+(8-r), resp->SwingUtilities.invokeLater(()->{
            parseLegal(resp);boardPanel.repaint();
        })); boardPanel.repaint();
    }

    private void parseLegal(String resp) {
        legalMoves.clear(); if(!resp.startsWith("LEGAL")) return;
        String[] p=resp.split("\\s+");
        for(int i=1;i<p.length;i++) if(p[i].length()>=2) {
            int c2=p[i].charAt(0)-'a', r2=8-(p[i].charAt(1)-'0');
            if(c2>=0&&c2<8&&r2>=0&&r2<8) legalMoves.add(new int[]{r2,c2});
        }
    }

    private void execMove(int fr,int fc,int tr,int tc) {
        String cmd="MOVE "+(char)('a'+fc)+(8-fr)+(char)('a'+tc)+(8-tc);
        // Fix: correct rank for target
        cmd="MOVE "+(char)('a'+fc)+(8-fr)+(char)('a'+tc)+(8-tr);
        if(Math.abs(board[fr][fc])==1&&(tr==0||tr==7)) cmd+=askPromo();
        lastFrom=new int[]{fr,fc}; lastTo=new int[]{tr,tc};
        final String c=cmd;
        engine.send(c, resp->{
            if(resp.startsWith("OK")) SwingUtilities.invokeLater(()->parseMoveResp(resp,fr,fc,tr,tc));
            else SwingUtilities.invokeLater(()->JOptionPane.showMessageDialog(this,"Illegal move!","Error",JOptionPane.ERROR_MESSAGE));
        });
    }

    private void parseMoveResp(String resp,int fr,int fc,int tr,int tc) {
        String fen="",fb="",ex="";
        int ei=resp.indexOf(" EVAL "),fi=resp.indexOf(" FEEDBACK "),xi=resp.indexOf(" EXPLAIN ");
        fen=resp.substring(3,ei>0?ei:resp.length()).trim();
        if(ei>0){int ee=fi>0?fi:(xi>0?xi:resp.length());try{currentEval=Integer.parseInt(resp.substring(ei+6,ee).trim());}catch(Exception e){}}
        if(fi>0){int fe=xi>0?xi:resp.length();fb=resp.substring(fi+10,fe).trim();}
        if(xi>0) ex=resp.substring(xi+9).trim();
        updateFEN(fen); recordMove(fr,fc,tr,tc,false); showCoach(fb,ex,false);
        evalBar.repaint(); checkStatus(); boardPanel.repaint();
        if(!gameStatus.equals("CHECKMATE")&&!gameStatus.equals("STALEMATE")) scheduleAI();
    }

    private String askPromo() {
        String[] o={"Queen","Rook","Bishop","Knight"};
        int c=JOptionPane.showOptionDialog(this,"Promote pawn to:","Pawn Promotion",
            JOptionPane.DEFAULT_OPTION,JOptionPane.QUESTION_MESSAGE,null,o,o[0]);
        switch(c){case 1:return"r";case 2:return"b";case 3:return"n";default:return"q";}
    }

    private void scheduleAI() {
        if(isPlayerTurn()) return; aiThinking=true; updateStatus();
        new SwingWorker<Void,Void>(){
            @Override protected Void doInBackground(){
                engine.send("AI "+aiDepth, resp->SwingUtilities.invokeLater(()->{
                    aiThinking=false;
                    if(resp.startsWith("BESTMOVE")) parseAI(resp);
                    updateStatus();
                })); return null;
            }
        }.execute();
    }

    private void parseAI(String resp) {
        String[] p=resp.split("\\s+"); if(p.length<2||p[1].length()<4) return;
        int fc=p[1].charAt(0)-'a',fr=8-(p[1].charAt(1)-'0'),tc=p[1].charAt(2)-'a',tr=8-(p[1].charAt(3)-'0');
        lastFrom=new int[]{fr,fc}; lastTo=new int[]{tr,tc};
        int si=resp.indexOf(" SCORE "),ni=resp.indexOf(" NODES "),xi=resp.indexOf(" EXPLAIN ");
        int fs=resp.indexOf(' ',9)+1, fe=si>0?si:resp.length();
        String fen=resp.substring(fs,fe).trim();
        if(si>0){int se=ni>0?ni:(xi>0?xi:resp.length());try{currentEval=Integer.parseInt(resp.substring(si+7,se).trim());}catch(Exception e){}}
        String ex=xi>0?resp.substring(xi+9).trim():"";
        updateFEN(fen); recordMove(fr,fc,tr,tc,true); showCoach(null,ex,true);
        evalBar.repaint(); checkStatus(); boardPanel.repaint();
    }

    private void undoMove() {
        if(aiThinking) return;
        engine.send("UNDO", r1->{
            if(r1.startsWith("OK")){
                engine.send("UNDO", r2->SwingUtilities.invokeLater(()->{
                    String fen=r2.startsWith("OK")?r2.substring(3).trim():r1.substring(3).trim();
                    updateFEN(fen); selRow=selCol=-1; legalMoves.clear(); lastFrom=lastTo=null;
                    coachPane.setText("Move taken back. Try a different approach!\n\nTip: Think about what your opponent might do after your move.");
                    boardPanel.repaint(); evalBar.repaint(); updateStatus();
                    engine.send("EVAL",er->{if(er.startsWith("EVAL "))try{currentEval=Integer.parseInt(er.substring(5).trim());SwingUtilities.invokeLater(()->evalBar.repaint());}catch(Exception e){}});
                    engine.send("STATUS",sr->SwingUtilities.invokeLater(()->{if(sr.startsWith("STATUS ")){gameStatus=sr.substring(7).trim();updateStatus();boardPanel.repaint();}}));
                }));
            } else SwingUtilities.invokeLater(()->coachPane.setText("Nothing to undo yet. Make a move first!"));
        });
    }

    private void updateFEN(String fen) {
        if(fen.isEmpty()) return; String[] p=fen.split(" ");
        for(int i=0;i<8;i++) for(int j=0;j<8;j++) board[i][j]=0;
        int row=0,col=0;
        for(char ch:p[0].toCharArray()){
            if(ch=='/'){row++;col=0;} else if(Character.isDigit(ch)) col+=ch-'0';
            else{if(row<8&&col<8) board[row][col]=ctp(ch);col++;}
        }
        whiteToMove=p.length>1&&p[1].equals("w");
    }

    private int ctp(char ch) {
        int t;switch(Character.toUpperCase(ch)){case'P':t=1;break;case'N':t=2;break;case'B':t=3;break;case'R':t=4;break;case'Q':t=5;break;case'K':t=6;break;default:return 0;}
        return Character.isUpperCase(ch)?t:-t;
    }

    private void checkStatus() {
        engine.send("STATUS", resp->SwingUtilities.invokeLater(()->{
            if(resp.startsWith("STATUS ")){
                gameStatus=resp.substring(7).trim(); updateStatus();
                if(gameStatus.equals("CHECKMATE")){String w=whiteToMove?"Black":"White";gameOver("Checkmate! "+w+" wins!");}
                else if(gameStatus.equals("STALEMATE")) gameOver("Stalemate! It's a draw.");
                boardPanel.repaint();
            }
        }));
    }

    private void updateStatus() {
        String t=whiteToMove?"White":"Black";
        if(gameStatus.equals("CHECKMATE")){String w=whiteToMove?"Black":"White";statusLabel.setText("Checkmate \u2013 "+w+" wins!");statusLabel.setForeground(DANGER);turnDot.setForeground(DANGER);}
        else if(gameStatus.equals("STALEMATE")){statusLabel.setText("Stalemate \u2013 Draw");statusLabel.setForeground(WARNING);turnDot.setForeground(WARNING);}
        else if(gameStatus.equals("CHECK")){statusLabel.setText(t+" is in CHECK!");statusLabel.setForeground(DANGER);turnDot.setForeground(DANGER);}
        else if(aiThinking){statusLabel.setText("AI is thinking\u2026");statusLabel.setForeground(TEXT_MUT);turnDot.setForeground(TEXT_MUT);}
        else{statusLabel.setText(t+" to move");statusLabel.setForeground(TEXT_PRI);turnDot.setForeground(whiteToMove?ACCENT:TEXT_PRI);}
    }

    private void recordMove(int fr,int fc,int tr,int tc,boolean ai) {
        String n=""+(char)('a'+fc)+(8-fr)+"-"+(char)('a'+tc)+(8-tr);
        if(!ai&&playerIsWhite||ai&&!playerIsWhite){
            if(!whiteMoved){histArea.append(String.format("%3d. %-10s",moveNumber,n));whiteMoved=true;}
            else{histArea.append("  "+n+"\n");moveNumber++;whiteMoved=false;}
        } else {
            if(!whiteMoved){histArea.append(String.format("%3d. ...       %s\n",moveNumber,n));moveNumber++;}
            else{histArea.append("  "+n+"\n");moveNumber++;whiteMoved=false;}
        }
        histArea.setCaretPosition(histArea.getDocument().getLength());
    }

    private void gameOver(String msg) {
        SwingUtilities.invokeLater(()->{
            coachPane.setText("\uD83C\uDFC1 "+msg+"\n\nClick 'New Game' to play again.");
            JOptionPane.showMessageDialog(this,msg,"Game Over",JOptionPane.INFORMATION_MESSAGE);
        });
    }

    private void newGame() {
        engine.send("NEW", resp->SwingUtilities.invokeLater(()->{
            if(resp.startsWith("OK")){
                updateFEN(resp.substring(3).trim());
                selRow=selCol=-1;legalMoves.clear();lastFrom=lastTo=null;
                gameStatus="NORMAL";currentEval=0;moveNumber=1;whiteMoved=false;
                histArea.setText("");boardFlipped=!playerIsWhite;
                coachPane.setText("New game started! Good luck.\n\nTip: Control the center with pawns, then develop knights and bishops.");
                updateStatus();boardPanel.repaint();evalBar.repaint();
                if(!playerIsWhite) scheduleAI();
            }
        }));
    }

    // ══════════════════════════════════════════════════════════
    //  ENGINE HANDLER
    // ══════════════════════════════════════════════════════════
    class EngineHandler {
        Process proc; BufferedWriter wr; BufferedReader rd;
        final ExecutorService ex=Executors.newSingleThreadExecutor();
        final ConcurrentLinkedQueue<java.util.function.Consumer<String>> cbs=new ConcurrentLinkedQueue<>();

        void start() {
            try {
                String ep=findEng(); ProcessBuilder pb=new ProcessBuilder(ep); pb.redirectErrorStream(false);
                proc=pb.start();
                wr=new BufferedWriter(new OutputStreamWriter(proc.getOutputStream()));
                rd=new BufferedReader(new InputStreamReader(proc.getInputStream()));
                Thread rt=new Thread(()->{try{String l;while((l=rd.readLine())!=null){final String r=l;java.util.function.Consumer<String> cb=cbs.poll();if(cb!=null)cb.accept(r);}}catch(IOException e){}});
                rt.setDaemon(true);rt.start();
                Thread et=new Thread(()->{try(BufferedReader err=new BufferedReader(new InputStreamReader(proc.getErrorStream()))){String l;while((l=err.readLine())!=null)System.err.println("[Engine] "+l);}catch(IOException i){}});
                et.setDaemon(true);et.start();
            } catch(IOException e){showEngErr(e.getMessage());}
        }

        String findEng() {
            for(String p:new String[]{"./chess_engine","./chess_engine.exe","../chess_engine","chess_engine"})
                if(new File(p).exists()) return p;
            return "./chess_engine";
        }

        void send(String cmd, java.util.function.Consumer<String> cb) {
            cbs.offer(cb);
            ex.submit(()->{try{wr.write(cmd+"\n");wr.flush();}catch(IOException e){cbs.poll();SwingUtilities.invokeLater(()->showEngErr("Write error: "+e.getMessage()));}});
        }

        void stop(){try{send("QUIT",r->{});if(proc!=null)proc.destroyForcibly();}catch(Exception i){}ex.shutdownNow();}
    }

    void showEngErr(String msg) {
        JOptionPane.showMessageDialog(this,"Could not connect to chess engine!\nMake sure 'chess_engine' is compiled.\n\nError: "+msg,"Engine Error",JOptionPane.ERROR_MESSAGE);
    }

    @Override public void dispose(){if(engine!=null)engine.stop();super.dispose();}

    // ══════════════════════════════════════════════════════════
    //  ROUNDED BORDER
    // ══════════════════════════════════════════════════════════
    static class RoundBdr extends AbstractBorder {
        final Color color; final int rad;
        RoundBdr(Color c,int r){color=c;rad=r;}
        @Override public void paintBorder(Component c,Graphics g,int x,int y,int w,int h){
            Graphics2D g2=(Graphics2D)g.create();
            g2.setRenderingHint(RenderingHints.KEY_ANTIALIASING,RenderingHints.VALUE_ANTIALIAS_ON);
            g2.setColor(color);g2.drawRoundRect(x,y,w-1,h-1,rad,rad);g2.dispose();
        }
        @Override public Insets getBorderInsets(Component c){return new Insets(2,2,2,2);}
        @Override public Insets getBorderInsets(Component c,Insets i){i.left=i.top=i.right=i.bottom=2;return i;}
    }
}

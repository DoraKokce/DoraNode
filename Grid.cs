using System.Numerics;
using Raylib_cs;

namespace DoraNode;

class Grid(int squareSize, int bigSq, int sqWidth, int sqHeight)
{
    public int selectedIndex = -1;
    List<Node> nodes = [];
    public Node? Selected => selectedIndex != -1 ? nodes[selectedIndex] : null;
    int _squareSize = squareSize, _bigSq = bigSq, _sqWidth = sqWidth, _sqHeight = sqHeight;
    public int xmax = sqWidth * squareSize, ymax = sqHeight * squareSize;
    Vector2 lastMousePos;

    public Color NodeBackgroundColor = new(46, 46, 46);
    public Color NodeSelectedColor = new(255, 150, 0);

    public void Draw(Font font)
    {
        DrawGrid();
        DrawNodes(font);
    }

    void DrawGrid()
    {
        for (int x = -_sqWidth; x < _sqWidth; x++)
        {
            Color color = x % _bigSq == 0 ? Color.Gray : Color.DarkGray;
            int xpos = x * _squareSize;
            Raylib.DrawLine(xpos, -ymax, xpos, ymax, color);
        }

        for (int y = -_sqHeight; y < _sqHeight; y++)
        {
            Color color = y % _bigSq == 0 ? Color.Gray : Color.DarkGray;
            int ypos = y * _squareSize;
            Raylib.DrawLine(-xmax, ypos, xmax, ypos, color);
        }
    }

    public void Update(Vector2 mousePos, Camera2D cam)
    {
        Vector2 mouseWorldPos = Raylib.GetScreenToWorld2D(mousePos, cam);
        if (CheckDragable(mouseWorldPos, cam)) return;
        for (int i = 0; i < nodes.Count; i++)
        {
            Node node = nodes[i];
            if (mouseWorldPos.X > node.Pos.X && mouseWorldPos.Y > node.Pos.Y &&
                mouseWorldPos.X < node.Pos.X + node.Size.X && mouseWorldPos.Y < node.Pos.Y + node.Size.Y)
            {
                selectedIndex = i;
                return;
            }
        }
        selectedIndex = -1;
    }

    bool CheckDragable(Vector2 worldMouse, Camera2D cam)
    {
        if (Raylib.IsMouseButtonPressed(MouseButton.Left) && Selected != null)
        {
            lastMousePos = Raylib.GetMousePosition(); return true;
        }
        else if (Raylib.IsMouseButtonDown(MouseButton.Left) && Selected != null)
        {
            Vector2 pos = worldMouse - Raylib.GetScreenToWorld2D(lastMousePos, cam);
            Vector2 newPos = Selected.Pos + pos;
            if (newPos.X < -xmax) newPos.X = -xmax;
            if (newPos.X + Selected.Size.X > xmax) newPos.X = xmax - Selected.Size.X;
            if (newPos.Y < -ymax) newPos.Y = -ymax;
            if (newPos.Y + Selected.Size.Y > ymax) newPos.Y = ymax - Selected.Size.Y;
            Selected.Pos = newPos;
            lastMousePos = Raylib.GetMousePosition();
            return true;
        }
        return false;
    }

    void DrawNodes(Font font)
    {
        foreach (var node in nodes)
        {
            DrawNode(node, font);
        }
    }

    void DrawNode(Node node, Font font)
    {
        DrawNodeBackground(node);
        Raylib.DrawTextEx(font, node.Title, node.Pos + new Vector2(15, 10), 20, 1, Color.White);
        if (selectedIndex == nodes.FindIndex((x) => node == x))
        {
            Raylib.DrawRectangleRoundedLines(new Rectangle(node.Pos, node.Size), .2f, 5, NodeSelectedColor);
        }
    }

    void DrawNodeBackground(Node node)
    {
        Raylib.DrawRectangleRounded(new Rectangle(node.Pos, node.Size), .2f, 5, NodeBackgroundColor);
        Raylib.DrawRectangleRounded(new Rectangle(node.Pos, node.Size.X, 40), 1.5f, 5, node.TitleColor);
        Raylib.DrawRectangle((int)node.Pos.X, (int)node.Pos.Y + 20, (int)node.Size.X, 20, node.TitleColor);
        Raylib.DrawRectangleRoundedLinesEx(new Rectangle(node.Pos, node.Size), .2f, 5, 5, node.TitleColor);
    }

    public bool InBounds(Vector2 a) => a.X > -xmax && a.X < xmax && a.Y > -ymax && a.Y < ymax;

    public void AddNode(Node node) => nodes.Add(node);
}
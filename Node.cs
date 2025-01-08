using System.Numerics;
using Raylib_cs;

namespace DoraNode;

public class Node(string title, Vector2 size, Color? titleBackColor = null)
{
    public Color TitleColor = titleBackColor ?? new(36, 36, 36);
    public string Title = title;
    public Vector2 Size = size;
    public Vector2 Pos;
}

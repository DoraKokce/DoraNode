using System.Numerics;
using Raylib_cs;

namespace DoraNode;

public class Window()
{
    Grid grid = new(20, 5, 80, 80);
    Color backColor = new(30, 30, 30);
    Vector2 prevMousePos, mousePos = Vector2.Zero;
    public void Init()
    {
        Raylib.InitWindow(800, 600, "deneme");
        Raylib.SetTargetFPS(80);
        Raylib.SetExitKey(KeyboardKey.Null);
        Raylib.SetWindowState(ConfigFlags.ResizableWindow);

        int count = 0;
        Font font = Raylib.LoadFontEx("Roboto.ttf", 20, Raylib.LoadCodepoints("ABCÇDEFGĞHIİJKLMNOÖPQRSŞTUÜVWXYZabcçdefgğhıijklmnoöpqrsştuüvwxyz@#$%+-*/\"'&{[]}=&1234567890", ref count), 0);
        Vector2 center = new(400, 300);
        Camera2D camera2D = new(center, Vector2.Zero, 0, 1);
        Node example = new("Çıkarma", new Vector2(200, 300));

        grid.AddNode(example);

        while (!Raylib.WindowShouldClose())
        {
            CameraLoop(ref camera2D);

            DrawLoop(camera2D, font);
        }

        Raylib.CloseWindow();
    }

    void CameraLoop(ref Camera2D cam)
    {
        mousePos = Raylib.GetMousePosition();
        if (Raylib.IsWindowResized()) cam.Offset = new(Raylib.GetScreenWidth() / 2, Raylib.GetScreenHeight() / 2);
        grid.Update(Raylib.GetMousePosition(), cam);
        if (grid.selectedIndex == -1) CameraMove(ref cam);
        CheckCamera(ref cam);
        prevMousePos = mousePos;
    }

    void DrawLoop(Camera2D cam, Font font)
    {
        Raylib.BeginDrawing();
        Raylib.BeginMode2D(cam);

        Raylib.ClearBackground(backColor);
        grid.Draw(font);

        Raylib.EndMode2D();
        Raylib.EndDrawing();
    }

    void CameraMove(ref Camera2D cam)
    {
        Vector2 delta = prevMousePos - mousePos;
        if (Raylib.IsMouseButtonDown(MouseButton.Left))
        {
            Vector2 target = Raylib.GetScreenToWorld2D(cam.Offset + delta, cam);
            cam.Target = target;
        }
    }

    void CheckCamera(ref Camera2D cam)
    {
        Vector2 target = cam.Target;
        if (GetCameraTopLeft(cam).X < -grid.xmax)
            target.X -= GetCameraTopLeft(cam).X + grid.xmax;
        if (GetCameraBottomRight(cam).X > grid.xmax)
            target.X -= GetCameraBottomRight(cam).X - grid.xmax;
        if (GetCameraTopLeft(cam).Y < -grid.ymax)
            target.Y -= GetCameraTopLeft(cam).Y + grid.ymax;
        if (GetCameraBottomRight(cam).Y > grid.ymax)
            target.Y -= GetCameraBottomRight(cam).Y - grid.ymax;
        cam.Target = target;
    }

    Vector2 GetCameraTopLeft(Camera2D cam) => Raylib.GetScreenToWorld2D(Vector2.Zero, cam);
    Vector2 GetCameraBottomRight(Camera2D cam) => Raylib.GetScreenToWorld2D(new Vector2(Raylib.GetScreenWidth(), Raylib.GetScreenHeight()), cam);
}

#include <libtcc.h>
#include <locale.h>
#include <math.h>
#include <raylib.h>
#include <raymath.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* --constants-- */
#define MAX_VARIABLES 100

#define NODE_COLOR (Color){0, 153, 255, 255}

#define MENU_BACK_COLOR WHITE
#define MENU_WIDTH 150

#define GRID_BIG_SQR 5
#define GRID_SQR_SIDE 20
#define GRID_BIG_W 16
#define GRID_BIG_H 9
#define GRID_SQR_COLOR LIGHTGRAY
#define GRID_BIG_COLOR DARKGRAY

#define NODE_TYPE_NAME                                                         \
  (char *[]){"Başla", "Bitir", "İşlem", "Değer", "Çağır",                      \
             "Giriş", "Çıkış", "Karar", "Döngü"}

#define NODE_TYPE_COLOR                                                        \
  (Color[]) {                                                                  \
    LIME, RED, NODE_COLOR, NODE_COLOR, NODE_COLOR, NODE_COLOR, NODE_COLOR,     \
        NODE_COLOR, NODE_COLOR                                                 \
  }

/* ---- */
typedef enum {
  NODE_START,
  NODE_END,
  NODE_PROCESS,
  NODE_VARIABLE,
  NODE_CALL,
  NODE_INPUT,
  NODE_OUTPUT,
  NODE_DECISION,
  NODE_LOOP,
} NodeType;

typedef struct Node {
  NodeType type;
  Vector2 position;
  char *text;
  Color instanceColor;
  bool isSelected;
  bool isEditing;
  bool editable;
  struct Node *next;
  struct Node *alt_next;
  unsigned int id;
} Node;

static Node *nodes = NULL;
static bool *visitedNodes = NULL;
static int nodeCount = 0;

typedef struct {
  char *name;
  char *type;
} Variable;

static Variable vars[MAX_VARIABLES];
static int var_count = 0;

static Node *linkingNode = NULL;
static bool linkingAlt = false;
static bool isLinking = false;

static bool draggingFromMenu = false;
static NodeType draggingType;

static Node *selectedNode = NULL;
static int selectedIndex = -1;

static Vector2 dragOffset = {0};
static bool isDragging = false;

static Node *editingNode = NULL;
static bool isEditing = false;

void DrawGridD(int sqrSide, int bigSqr, int bigSqrCW, int bigSqrCH,
               Color sqrColor, Color bigSqrColor);

Node CreateNode(NodeType type, Vector2 pos);
void AddNode(NodeType type, Vector2 pos);
void DeleteNode(int index);

void DrawNode(Node *node, Font font);
void DrawNodePreview(NodeType type, Font font, Vector2 pos);
void DrawLink(Node *node, Font font);
void DrawArrow(Vector2 start, Vector2 end, Color color);
void DrawLabelOnLine(Vector2 start, Vector2 end, const char *text, Font font,
                     Color color);
Vector2 GetClosestEdge(Node *startNode, Node *destNode, Font font);

void DrawMenu(Color back, Font font);

Font LoadFontT();
char *CompileCode(Node *node, char *textBefore);
char *CompileCodeToEXE(Node *node, char *fileName);
char *append_string(char *base, const char *addition);

void BackspaceUTF8(char *text) {
  int len = strlen(text);
  if (len == 0)
    return;

  int i = len - 1;
  while (i > 0 && ((text[i] & 0xC0) == 0x80)) {
    i--;
  }

  text[i] = '\0';
}

Node *IfTypeExist(NodeType type) {
  for (int i = 0; i < nodeCount; i++) {
    if (nodes[i].type == type)
      return &nodes[i];
  }
  return NULL;
}

int main(void) {
  setlocale(LC_ALL, "Turkish");

  InitWindow(800, 600, "DoraNode test 1.5");
  SetWindowState(FLAG_WINDOW_RESIZABLE);
  Font font = LoadFontT();
  Camera2D cam = (Camera2D){Vector2Zero(), Vector2Zero(), 0, 1};
  cam.offset = (Vector2){GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};

  SetTargetFPS(60);
  SetExitKey(0);

  Texture2D trashIcon = LoadTexture("resources/trash-icon.png"),
            runButtonTex = LoadTexture("resources/start.png");

  Vector2 prevMousePos;
  Vector2 trashPos = {GetScreenWidth() - 53, GetScreenHeight() - 53},
          runPosButton = {GetScreenWidth() - 53, 0};

  while (!WindowShouldClose()) {
    Vector2 mousePos = GetMousePosition();
    Vector2 worldMouse = GetScreenToWorld2D(mousePos, cam);
    if (IsWindowResized()) {
      cam.offset = (Vector2){GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};
      trashPos = (Vector2){GetScreenWidth() - 53, GetScreenHeight() - 53};
      runPosButton = (Vector2){GetScreenWidth() - 53, 0};
    }

    static double lastClickTime = 0;
    double currentTime = GetTime();
    bool doubleClick = false;
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      if (selectedNode != NULL) {
        if (currentTime - lastClickTime < 0.3) {
          doubleClick = true;
        }
        lastClickTime = currentTime;
      }
    }

    if (doubleClick && selectedNode != NULL && selectedNode->editable) {
      editingNode = selectedNode;
      isEditing = true;
    }

    if (isEditing) {
      int key = GetCharPressed();

      while (key > 0) {
        if (key >= 32) {
          char utf8[5] = {
              0}; // UTF-8 karakterler en fazla 4 byte + null terminator
          int utf8Size = 0;
          const char *converted = CodepointToUTF8(key, &utf8Size);
          memcpy(utf8, converted, utf8Size);
          utf8[utf8Size] = '\0';

          editingNode->text = append_string(editingNode->text, utf8);
        }
        key = GetCharPressed(); // birden fazla tuş varsa sırayla al
      }

      if (IsKeyPressed(KEY_BACKSPACE)) {
        BackspaceUTF8(editingNode->text);
      }

      if (selectedNode != editingNode) {
        isEditing = false;
        editingNode = NULL;
      }
    }

    if (mousePos.x > MENU_WIDTH && !isDragging && !draggingFromMenu &&
        nodeCount > 0) {
      for (int i = nodeCount - 1; i >= 0; i--) {
        Node *node = &nodes[i];
        float width = 100 + MeasureTextEx(font, (char *)node->text, 20, 1).x,
              height = 50;

        Rectangle rect = {node->position.x - width / 2,
                          node->position.y - height / 2, width, height};

        if (CheckCollisionPointRec(worldMouse, rect)) {
          selectedNode = node;
          selectedIndex = i;
          break;
        } else {
          selectedNode = NULL;
          selectedIndex = -1;
        }
      }
    }

    if (mousePos.x > trashPos.x - 10 && mousePos.y > trashPos.y - 10 &&
        selectedNode != NULL) {
      DeleteNode(selectedIndex);
      selectedNode = NULL;
      selectedIndex = -1;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && selectedNode != NULL) {
      isLinking = true;
      linkingNode = selectedNode;

      if (linkingNode->next != NULL && (linkingNode->type == NODE_DECISION ||
                                        linkingNode->type == NODE_LOOP)) {
        linkingAlt = true;
      } else {
        linkingAlt = false;
      }
    } else if (IsMouseButtonReleased(MOUSE_BUTTON_RIGHT) && isLinking) {
      if (selectedNode != NULL && selectedNode->type != NODE_START) {
        if (linkingAlt)
          linkingNode->alt_next = selectedNode;
        else
          linkingNode->next = selectedNode;
      }

      isLinking = false;
      linkingAlt = false;
      linkingNode = NULL;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && selectedNode != NULL) {
      isDragging = true;
      dragOffset = Vector2Subtract(selectedNode->position, worldMouse);
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
        mousePos.x > runPosButton.x - 10 && mousePos.y < 63) {
      CompileCodeToEXE(IfTypeExist(NODE_START), "temp");
    }

    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
      isDragging = false;
    }

    if (isDragging && selectedNode != NULL) {
      selectedNode->position = Vector2Add(worldMouse, dragOffset);
    }
    if (draggingFromMenu && mousePos.x > MENU_WIDTH) {
      if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        AddNode(draggingType, worldMouse);

        draggingFromMenu = false;
      }
    }

    prevMousePos = mousePos;

    BeginDrawing();
    BeginMode2D(cam);

    ClearBackground(WHITE);

    DrawGridD(GRID_SQR_SIDE, GRID_BIG_SQR, GRID_BIG_W, GRID_BIG_H,
              GRID_SQR_COLOR, GRID_BIG_COLOR);

    if (isLinking) {
      if (selectedNode != NULL && selectedNode != linkingNode) {
        DrawArrow(linkingNode->position,
                  GetClosestEdge(linkingNode, selectedNode, font), ORANGE);
      } else {
        DrawArrow(linkingNode->position, worldMouse, ORANGE);
      }
    }

    for (int i = 0; i < nodeCount; i++) {
      DrawLink(&nodes[i], font);
      DrawNode(&nodes[i], font);
    }

    if (draggingFromMenu) {
      DrawNodePreview(draggingType, font, worldMouse);
    }

    EndMode2D();

    DrawMenu(MENU_BACK_COLOR, font);

    DrawRectangle(runPosButton.x - 10, runPosButton.y, 63, 58, GREEN);
    DrawTextureEx(runButtonTex, Vector2Add(runPosButton, (Vector2){0, 5}), 0,
                  0.75, WHITE);

    DrawRectangle(trashPos.x - 10, trashPos.y - 10, 63, 63, RED);
    DrawTextureV(trashIcon, trashPos, WHITE);

    EndDrawing();
  }

  UnloadFont(font);
  UnloadTexture(trashIcon);

  CloseWindow();
}

void DrawMenu(Color back, Font font) {
  DrawRectangle(0, 0, MENU_WIDTH, GetScreenHeight(), back);
  DrawRectangleLines(0, 0, MENU_WIDTH, GetScreenHeight(), BLACK);

  Vector2 startPos = {75, 40}, incrementPos = {0, 65},
          mousePos = GetMousePosition();
  for (NodeType i = NODE_START; i <= NODE_LOOP; i++) {
    Vector2 pos = Vector2Add(startPos, Vector2Scale(incrementPos, i));
    DrawNodePreview(i, font, pos);
    if (i != NODE_LOOP)
      DrawLine(0, pos.y + 32, MENU_WIDTH, pos.y + 32, BLACK);
    for (NodeType i = NODE_START; i <= NODE_LOOP; i++) {
      Vector2 pos = Vector2Add(startPos, Vector2Scale(incrementPos, i));
      Rectangle rect = {pos.x - 50, pos.y - 25, 100, 50};
      if (selectedNode == NULL && !draggingFromMenu &&
          CheckCollisionPointRec(mousePos, rect) &&
          IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        draggingFromMenu = true;
        draggingType = i;
        break;
      }
    }
  }
}

Font LoadFontT() {

  int codepoints[] = {
      32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,
      46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,
      60,  61,  62,  63,  64,  65,  66,  67,  68,  69,  70,  71,  72,  73,
      74,  75,  76,  77,  78,  79,  80,  81,  82,  83,  84,  85,  86,  87,
      88,  89,  90,  91,  92,  93,  94,  95,  96,  97,  98,  99,  100, 101,
      102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115,
      116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126,

      199, 214, 220, 304, 286, 350, 231, 246, 252, 287, 305, 351};

  int codepointCount = sizeof(codepoints) / sizeof(codepoints[0]);

  // Fontu yükle
  Font font =
      LoadFontEx("resources/OpenSans.ttf", 20, codepoints, codepointCount);

  return font;
}

void AddNode(NodeType type, Vector2 pos) {
  Node newNode = CreateNode(type, pos);

  Node *newNodes = realloc(nodes, (nodeCount + 1) * sizeof(Node));
  bool *newVisitedNodes = realloc(visitedNodes, (nodeCount + 1) * sizeof(bool));
  if (!newNodes || !newVisitedNodes) {
    printf("Bellek tahsisi başarısız!\n");
    exit(1);
  }

  nodes = newNodes;
  visitedNodes = newVisitedNodes;

  nodes[nodeCount] = newNode;
  visitedNodes[nodeCount] = false;
  nodeCount++;
}

Node CreateNode(NodeType type, Vector2 pos) {
  Node node = {.type = type,
               .position = pos,
               .text = strdup(NODE_TYPE_NAME[type]),
               .instanceColor = NODE_TYPE_COLOR[type],
               .isSelected = false,
               .isEditing = false,
               .editable = (type != NODE_START && type != NODE_END),
               .id = nodeCount};
  return node;
}

void DeleteNode(int index) {
  if (index < 0 || index >= nodeCount) {
    printf("Geçersiz indeks!\n");
    return;
  }

  Node *deletingNode = &nodes[index];

  for (int i = 0; i < nodeCount; i++) {
    if (nodes[i].next == deletingNode)
      nodes[i].next = NULL;
    if (nodes[i].alt_next == deletingNode)
      nodes[i].alt_next = NULL;
  }

  free(deletingNode->text);

  for (int i = index; i < nodeCount - 1; i++) {
    nodes[i] = nodes[i + 1];
    nodes[i].id = i;
    visitedNodes[i] = visitedNodes[i + 1];
  }

  nodeCount--;

  if (nodeCount > 0) {
    Node *newNodes = realloc(nodes, nodeCount * sizeof(Node));
    bool *newVisited = realloc(visitedNodes, nodeCount * sizeof(bool));
    if (!newNodes || !newVisited) {
      printf("Yeniden tahsis hatası!\n");
      exit(1);
    }
    nodes = newNodes;
    visitedNodes = newVisited;
  } else {
    free(nodes);
    free(visitedNodes);
    nodes = NULL;
    visitedNodes = NULL;
  }
}

void DrawGridD(int sqrSide, int bigSqr, int bigSqrCW, int bigSqrCH,
               Color sqrColor, Color bigSqrColor) {
  int bigW = bigSqr * bigSqrCW * sqrSide;
  int bigH = bigSqr * bigSqrCH * sqrSide;

  for (int x = -bigW; x <= bigW; x += sqrSide) {
    if (x % (bigSqr * sqrSide) == 0) {
      DrawLineEx((Vector2){x, -bigH}, (Vector2){x, bigH}, 1, bigSqrColor);
    } else
      DrawLine(x, -bigH, x, bigH, sqrColor);
  }

  for (int y = -bigH; y <= bigH; y += sqrSide) {
    if (y % (bigSqr * sqrSide) == 0) {
      DrawLineEx((Vector2){-bigW, y}, (Vector2){bigW, y}, 1, bigSqrColor);
    } else
      DrawLine(-bigW, y, bigW, y, sqrColor);
  }
}

void DrawNodePreview(NodeType type, Font font, Vector2 pos) {
  Node node = CreateNode(type, pos);
  DrawNode(&node, font);
}

void DrawNode(Node *node, Font font) {
  float textWidth = MeasureTextEx(font, (char *)node->text, 20, 1).x;
  float width = fmaxf(100, 20 + textWidth), height = 50 + textWidth / 10;
  Vector2 pos = node->position;
  Color outlineColor = node->isSelected || node->isEditing ? ORANGE : BLACK;
  switch (node->type) {
  case NODE_START:
  case NODE_END: {
    DrawRectangleRounded(
        (Rectangle){pos.x - width / 2, pos.y - height / 2, width, height}, 1,
        10, node->instanceColor);
    DrawRectangleRoundedLines(
        (Rectangle){pos.x - width / 2, pos.y - height / 2, width, height}, 1,
        10, outlineColor);
    break;
  }
  case NODE_PROCESS:
  case NODE_VARIABLE: {
    DrawRectangle(pos.x - width / 2, pos.y - height / 2, width, height,
                  node->instanceColor);
    DrawRectangleLines(pos.x - width / 2, pos.y - height / 2, width, height,
                       outlineColor);
    break;
  }
  case NODE_CALL: {
    DrawRectangle(pos.x - width / 2, pos.y - height / 2, width, height,
                  node->instanceColor);
    DrawRectangleLines(pos.x - width / 2, pos.y - height / 2, width, height,
                       outlineColor);
    DrawRectangleLines(pos.x - width / 2 + 15, pos.y - height / 2, width - 30,
                       height, outlineColor);
    break;
  }
  case NODE_INPUT:
  case NODE_OUTPUT: {
    Vector2 p1 = {pos.x - width / 2, pos.y - height / 2},
            p2 = {pos.x + width * .6f, pos.y - height / 2},
            p3 = {pos.x + width / 2, pos.y + height / 2},
            p4 = {pos.x - width * .6f, pos.y + height / 2};
    DrawTriangleStrip((Vector2[]){p2, p1, p3, p4}, 4, node->instanceColor);
    DrawLineStrip((Vector2[]){p1, p2, p3, p4, p1}, 5, outlineColor);
    break;
  }
  case NODE_DECISION: {
    Vector2 p1 = {pos.x, pos.y - height / 2}, p2 = {pos.x + width / 2, pos.y},
            p3 = {pos.x, pos.y + height / 2}, p4 = {pos.x - width / 2, pos.y};
    DrawTriangleStrip((Vector2[]){p2, p1, p3, p4}, 4, node->instanceColor);
    DrawLineStrip((Vector2[]){p1, p2, p3, p4, p1}, 5, outlineColor);
    break;
  }
  case NODE_LOOP: {
    Vector2 p1 = {pos.x - width / 2, pos.y},
            p2 = {pos.x - width / 4, pos.y + height / 2},
            p3 = {pos.x + width / 4, pos.y + height / 2},
            p4 = {pos.x + width / 2, pos.y},
            p5 = {pos.x + width / 4, pos.y - height / 2},
            p6 = {pos.x - width / 4, pos.y - height / 2};

    DrawTriangleFan((Vector2[]){pos, p1, p2, p3, p4, p5, p6, p1}, 8,
                    node->instanceColor);
    DrawLineStrip((Vector2[]){p1, p2, p3, p4, p5, p6, p1}, 7, outlineColor);
    break;
  }

  default:
    break;
  }
  DrawTextEx(font, (char *)node->text,
             (Vector2){pos.x - textWidth / 2, pos.y - 10}, 20, 1, WHITE);
}

void DrawLink(Node *node, Font font) {
  Vector2 arrow;
  if (node->next) {
    Vector2 end = GetClosestEdge(node, node->next, font);
    DrawArrow(node->position, end, ORANGE);
    if (node->alt_next != NULL) {
      DrawLabelOnLine(node->position, end, "Evet ise", font, BLACK);
    }
  }
  if (node->alt_next) {
    Vector2 end = GetClosestEdge(node, node->alt_next, font);
    DrawArrow(node->position, end, ORANGE);
    if (node->next != NULL) {
      DrawLabelOnLine(node->position, end, "Hayır ise", font, BLACK);
    }
  }
}

void DrawArrow(Vector2 start, Vector2 end, Color color) {
  DrawLineEx(start, end, 2.0f, color);

  Vector2 dir = Vector2Normalize(Vector2Subtract(end, start));
  float angle = atan2f(dir.y, dir.x);
  float arrowLength = 10.0f;
  float arrowAngle = PI / 6.0f; // 30 derece

  Vector2 left = {end.x - arrowLength * cosf(angle - arrowAngle),
                  end.y - arrowLength * sinf(angle - arrowAngle)};

  Vector2 right = {end.x - arrowLength * cosf(angle + arrowAngle),
                   end.y - arrowLength * sinf(angle + arrowAngle)};

  DrawLineEx(end, left, 2, color);
  DrawLineEx(end, right, 2, color);
}

void DrawLabelOnLine(Vector2 start, Vector2 end, const char *text, Font font,
                     Color color) {
  Vector2 mid = {(start.x + end.x) / 2.0f, (start.y + end.y) / 2.0f};

  float angle = atan2f(end.y - start.y, end.x - start.x) * RAD2DEG;

  Vector2 textSize = MeasureTextEx(font, text, 20, 1);
  Vector2 origin = {textSize.x / 2.0f, textSize.y / 2.0f};

  DrawTextPro(font, text, mid, origin, angle, 20, 0, color);
}

Vector2 GetClosestEdge(Node *startNode, Node *destNode, Font font) {
  float textWidth = MeasureTextEx(font, (char *)destNode->text, 20, 1).x;
  float width = fmaxf(100, 20 + textWidth), height = 50 + textWidth / 10;

  Vector2 edges[4] = {
      {destNode->position.x + width / 2, destNode->position.y},  // sağ
      {destNode->position.x - width / 2, destNode->position.y},  // sol
      {destNode->position.x, destNode->position.y + height / 2}, // alt
      {destNode->position.x, destNode->position.y - height / 2}  // üst
  };

  float minDist = Vector2DistanceSqr(startNode->position, edges[0]);
  Vector2 final = edges[0];

  for (int i = 1; i < 4; i++) {
    float dist = Vector2DistanceSqr(startNode->position, edges[i]);
    if (dist < minDist) {
      minDist = dist;
      final = edges[i];
    }
  }

  return final;
}

char *strprintf(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  // Uzunluğu öğrenmek için bir kopya argüman listesi gerekir
  va_list args_copy;
  va_copy(args_copy, args);
  int len = vsnprintf(NULL, 0, fmt, args_copy);
  va_end(args_copy);

  if (len < 0) {
    va_end(args);
    return NULL; // Hata durumu
  }

  char *str = malloc(len + 1); // +1 for the null terminator
  if (!str) {
    va_end(args);
    return NULL; // Bellek hatası
  }

  vsnprintf(str, len + 1, fmt, args);
  va_end(args);

  return str; // Döndürülen belleği serbest bırakmayı unutma
}

char *append_string(char *base, const char *addition) {
  if (!base)
    base = "";
  if (!addition)
    addition = "";

  size_t len1 = strlen(base);
  size_t len2 = strlen(addition);

  char *result =
      malloc((len1 + len2 + 1) * sizeof(char)); // +1 for the null terminator
  if (!result)
    return NULL;

  strcpy(result, base);
  strcat(result, addition);

  return result;
}

void addToVars(char *type, char *name) {
  vars[var_count++] = (Variable){.name = strdup(name), .type = strdup(type)};
}

char *CompileVar(Node *node) {
  char type[32] = {0};
  int i = 0, t_index = 0;

  while (node->text[i] && node->text[i] != ' ') {
    if (t_index < sizeof(type) - 1)
      type[t_index++] = node->text[i++];
  }
  type[t_index] = '\0';

  while (node->text[i] == ' ')
    i++;

  char name[64] = {0};
  int n_index = 0;

  for (; node->text[i]; i++) {
    char c = node->text[i];

    if (c == ',') {
      name[n_index] = '\0';
      addToVars(type, name);
      n_index = 0;
      memset(name, 0, sizeof(name));
      type[t_index--] = '\0';
    } else if (c == '*') {
      type[t_index] = '*';
      type[t_index++] = '\0';
    } else if (c != ' ') {
      if (n_index < sizeof(name) - 1)
        name[n_index++] = c;
    }
  }

  if (n_index > 0) {
    name[n_index] = '\0';
    addToVars(type, name);
  }

  return (char *)strprintf("\t%s;\n", node->text);
}

Variable *isdefined(char *name) {
  for (int i = 0; i < var_count; i++) {
    if (strcmp(vars[i].name, name) == 0)
      return &vars[i];
  }
  return NULL;
}

bool cmpStr(char *a, char *b) { return strcmp(a, b) == 0; }

char *getScanfFormat(char *type) {
  return cmpStr(type, "string") || cmpStr(type, "char *") ? "%s"
         : cmpStr(type, "char")                           ? "%c"
         : cmpStr(type, "int")                            ? "%d"
         : cmpStr(type, "float")                          ? "%f"
         : cmpStr(type, "double")                         ? "%lf"
         : cmpStr(type, "long")                           ? "%ld"
                                                          : "";
}

char *CompileInput(Node *node) {
  char *nodeText = node->text;
  char prompt[256] = {0};
  char varname[64] = {0};

  const char *quote_start = strchr(nodeText, '\"');
  const char *quote_end = strrchr(nodeText, '\"');
  if (!quote_start || !quote_end || quote_start == quote_end)
    return NULL;

  int prompt_len = quote_end - quote_start - 1;
  strncpy(prompt, quote_start + 1, prompt_len);
  prompt[prompt_len] = '\0';

  const char *comma = strchr(quote_end, ',');
  if (!comma)
    return NULL;

  while (*comma == ',' || *comma == ' ')
    comma++;
  strncpy(varname, comma, sizeof(varname) - 1);
  varname[sizeof(varname) - 1] = '\0';

  Variable *var = isdefined(varname);
  if (!var) {
    fputs("Değer tanımlanmamış!", stdout);
    return "<DORANODEHATA>";
  }
  char *type = var->type;

  if (strcmp(type, "char*") == 0 || strcmp(type, "string") == 0) {
    return strprintf("printf(\"%s\");\nfgets(%s, sizeof(%s), "
                     "stdin);\n%s[strcspn(%s, \"\\n\")] = 0;\n",
                     prompt, varname, varname, varname, varname);
  } else {
    char *format = getScanfFormat(type);
    return strprintf("printf(\"%s\");\nscanf(\"%s\", &%s);\n", prompt, format,
                     varname);
  }
}

char *CompileLoop(Node *node) {
  char *text;
  int semicolonCount = 0;
  for (int i = 0; node->text[i] != '\0'; i++) {
    if (node->text[i] == ';')
      semicolonCount++;
  }

  if (semicolonCount == 0) {
    text = (char *)strprintf("\twhile (%s) {\n", node->text);
  } else {
    text = (char *)strprintf("\tfor (%s) {\n", node->text);
  }

  text = CompileCode(node->next, text);
  text = append_string(text, "\t}\n");
  text = CompileCode(node->alt_next, text);

  return (char *)text;
}

char *CompileCode(Node *node, char *textBefore) {
  if (node == NULL)
    return "<DORANODEHATA>";

  char *text = textBefore;

  if (node->type == NODE_START) {
    text = append_string(text, "int main(void) {\n");
  }

  if (visitedNodes[node->id] == true && node->type == NODE_LOOP) {
    text = append_string(text, "\tcontinue;\n");
    return text;
  } else if (visitedNodes[node->id] == true) {
    text = append_string(text,
                         (char *)strprintf("\tgoto doraNode_%i;\n", node->id));
    return text;
  }

  visitedNodes[node->id] = true;

  if (node->type != NODE_START)
    text = append_string(text, (char *)strprintf("doraNode_%i:\n", node->id));
  switch (node->type) {
  case NODE_START:
    text = CompileCode(node->next, text);
    break;
  case NODE_END:
    text = append_string(text, "\treturn 0;\n}\n");
    break;
  case NODE_INPUT:
    text = append_string(text, CompileInput(node));
    text = CompileCode(node->next, text);
    break;
  case NODE_OUTPUT:
    text =
        append_string(text, (char *)strprintf("\tprintf(%s);\n", node->text));
    text = CompileCode(node->next, text);
    break;
  case NODE_VARIABLE:
    text = append_string(text, CompileVar(node));
    text = CompileCode(node->next, text);
    break;
  case NODE_DECISION:
    text = append_string(
        text, (char *)strprintf(
                  "\tif (%s) goto doraNode_%i;\n\telse goto doraNode_%i;\n",
                  node->text, node->next->id, node->alt_next->id));
    text = CompileCode(node->next, text);
    text = CompileCode(node->alt_next, text);
    break;
  case NODE_LOOP:
    text = append_string(text, CompileLoop(node));
    break;
  default:
    text = append_string(text, (char *)strprintf("\t%s;\n", node->text));
    text = CompileCode(node->next, text);
    break;
  }

  return text;
}

char *CompileCodeToEXE(Node *node, char *fileName) {
  memset(visitedNodes, 0, nodeCount * sizeof(bool));
  char *code = (char *)CompileCode(
      node, "#include <stdio.h>\n#include <stdbool.h>\n#include "
            "<math.h>\n#include <string.h>\n\ntypedef char* string;\n\n");

  printf("%s\n", code);

  TCCState *s = tcc_new();
  if (!s) {
    fprintf(stderr, "Failed to create TCC state\n");
    return NULL;
  }

  tcc_add_include_path(s, "/usr/include");
  tcc_add_include_path(s, "/usr/include/x86_64-linux-gnu");
  tcc_add_include_path(s, "/usr/lib/gcc/x86_64-linux-gnu/13/include");

  tcc_set_output_type(s, TCC_OUTPUT_EXE);

  tcc_add_library(s, "m");

  if (tcc_compile_string(s, code) == -1) {
    fprintf(stderr, "TCC compile error\n");
    tcc_delete(s);
    return NULL;
  }

  if (tcc_output_file(s, fileName) == -1) {
    fprintf(stderr, "TCC output error\n");
    tcc_delete(s);
    return NULL;
  }

  tcc_delete(s);
  return code;
}

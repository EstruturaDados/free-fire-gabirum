#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>

static void limpar_tela(void)
{
  HANDLE console_handle = GetStdHandle(STD_OUTPUT_HANDLE);

  if (console_handle == INVALID_HANDLE_VALUE)
    return;

  CONSOLE_SCREEN_BUFFER_INFO csbi;
  if (!GetConsoleScreenBufferInfo(console_handle, &csbi))
    return;

  DWORD cell_count = csbi.dwSize.X * csbi.dwSize.Y;
  COORD home_coords = {0, 0};
  DWORD count = 0;
  if (!FillConsoleOutputCharacter(console_handle, (TCHAR)' ', cell_count, home_coords, &count))
    return;

  if (!FillConsoleOutputAttribute(console_handle, csbi.wAttributes, cell_count, home_coords, &count))
    return;

  SetConsoleCursorPosition(console_handle, home_coords);
}
#else
#define limpar_tela() printf("\e[1;1H\e[2J")
#endif

#define limpar_input_buffer() scanf("%*c")

typedef struct item
{
  char nome[30], tipo[20];
  int quantidade, prioridade;
} item_t;

typedef enum criterio_ordenacao
{
  ORDENACAO_NAO_ORDENADO,
  ORDENACAO_POR_NOME,
  ORDENACAO_POR_TIPO,
  ORDENACAO_POR_PRIORIDADE
} criterio_ordenacao_e;

typedef struct mochila
{
  item_t *itens;
  int capacidade, num_itens;
  criterio_ordenacao_e criterio_atual;
} mochila_t;

static char const *const MENU = "Ações:\n"
                                "\t1. Adicionar um item\n"
                                "\t2. Remover um item\n"
                                "\t3. Listar todos os itens\n"
                                "\t4. Ordenar os itens por critério\n"
                                "\t5. Realizar busca binária por nome\n"
                                "\t0. Sair";
static char const *const MENU_ORDENACAO = "Ordenar por:\n"
                                          "\t1. Nome\n"
                                          "\t2. Tipo\n"
                                          "\t3. Prioridade";

static void exibir_menu(void);
static void esperar_enter(void);
static void inserir_item(mochila_t *mochila);
static void remover_item(mochila_t *mochila);
static void listar_itens(mochila_t *mochila);
static void ordenar_itens(mochila_t *mochila);
static item_t *busca_binaria_por_nome(mochila_t *mochila, char const *nome);
static void exibir_item(item_t *item);

int main(void)
{
  mochila_t mochila;
  mochila.itens = malloc(10 * sizeof(item_t));
  if (!mochila.itens)
  {
    perror("Erro ao alocar memória para a mochila");
    return 1;
  }
  mochila.capacidade = 10;
  mochila.num_itens = 0;
  mochila.criterio_atual = ORDENACAO_NAO_ORDENADO;

  int op = 0;
  do
  {
    exibir_menu();
    putchar('>');
    scanf("%d", &op);

    limpar_tela();
    switch (op)
    {
    case 1:
      inserir_item(&mochila);
      esperar_enter();
      break;

    case 2:
      remover_item(&mochila);
      esperar_enter();
      break;

    case 3:
      listar_itens(&mochila);
      esperar_enter();
      break;

    case 4:
      ordenar_itens(&mochila);
      break;

    case 5:
    {
      puts("Nome do item a procurar:");
      char nome[30];
      limpar_input_buffer();
      scanf("%29[^\n]", nome);
      if (nome[0] == 0)
      {
        fputs("Nome inserido vazio!\n", stderr);
        return;
      }
      item_t *item = busca_binaria_por_nome(&mochila, nome);
      if (item)
      {
        exibir_item(item);
      }
      else
      {
        fputs("Item não encontrado\n", stderr);
      }
      esperar_enter();
    }
    break;

    default:
      break;
    }
  } while (op != 0);

  free(mochila.itens);

  return 0;
}

static void exibir_menu(void)
{
  limpar_tela();
  puts(MENU);
}

void esperar_enter(void)
{
  puts("Pressione Enter para continuar...");
  limpar_input_buffer();
  getchar();
}

static bool garantir_capacidade(mochila_t *mochila, int capacidade)
{
  if (mochila->capacidade >= capacidade)
  {
    return true;
  }

  int nova_capacidade = mochila->capacidade + (mochila->capacidade >> 1); // 1.5x
  nova_capacidade = nova_capacidade < capacidade ? capacidade : nova_capacidade;

  item_t *novos_itens = realloc(mochila->itens, nova_capacidade * sizeof(item_t));
  if (!novos_itens)
  {
    perror("Erro ao realocar memória para a mochila");
    return false;
  }

  mochila->itens = novos_itens;
  mochila->capacidade = nova_capacidade;
  return true;
}

static bool item_existe(mochila_t *mochila, char const *nome)
{
  for (int i = 0; i < mochila->num_itens; i++)
  {
    if (strcmp(mochila->itens[i].nome, nome) == 0)
    {
      return true;
    }
  }

  return false;
}

static void inserir_item(mochila_t *mochila)
{
  if (!garantir_capacidade(mochila, mochila->num_itens + 1))
  {
    return;
  }

  puts("Nome do item:");
  limpar_input_buffer();
  scanf("%29[^\n]", mochila->itens[mochila->num_itens].nome);
  if (mochila->itens[mochila->num_itens].nome[0] == 0)
  {
    fputs("Nome inválido!\n", stderr);
    return;
  }
  if (item_existe(mochila, mochila->itens[mochila->num_itens].nome))
  {
    fputs("Item já existe na mochila!\n", stderr);
    return;
  }

  puts("Tipo do item:");
  limpar_input_buffer();
  scanf("%19[^\n]", mochila->itens[mochila->num_itens].tipo);
  if (mochila->itens[mochila->num_itens].tipo[0] == 0)
  {
    fputs("Tipo inválido!\n", stderr);
    return;
  }

  puts("Quantidade do item:");
  int quantidade = 0;
  limpar_input_buffer();
  scanf("%d", &quantidade);
  if (quantidade < 1)
  {
    fputs("Quantidade inválida!\n", stderr);
    return;
  }
  mochila->itens[mochila->num_itens].quantidade = quantidade;

  puts("Prioridade do item (1-5):");
  int prioridade = 0;
  limpar_input_buffer();
  scanf("%d", &prioridade);
  if (prioridade < 1 || prioridade > 5)
  {
    fputs("Prioridade inválida!\n", stderr);
    return;
  }
  mochila->itens[mochila->num_itens].prioridade = prioridade;
  mochila->num_itens++;
  mochila->criterio_atual = ORDENACAO_NAO_ORDENADO;
}

static void remover_item(mochila_t *mochila)
{
  if (mochila->num_itens == 0)
  {
    fputs("Mochila vazia!\n", stderr);
    return;
  }

  puts("Nome do item a remover:");
  char nome[30];
  limpar_input_buffer();
  scanf("%29[^\n]", nome);
  if (nome[0] == 0)
  {
    fputs("Nome inserido vazio!\n", stderr);
    return;
  }

  for (int i = 0; i < mochila->num_itens; i++)
  {
    if (strcmp(mochila->itens[i].nome, nome) == 0)
    {
      if (i < mochila->num_itens - 1)
      {
        memmove(&mochila->itens[i], &mochila->itens[i + 1], (mochila->num_itens - i - 1) * sizeof(item_t));
      }
      mochila->num_itens--;
      mochila->criterio_atual = ORDENACAO_NAO_ORDENADO;
      return;
    }
  }

  fputs("Item não encontrado na mochila!\n", stderr);
}

#define print_item_header() printf("%-30s %-20s %10s %10s\n", "Nome", "Tipo", "Quantidade", "Prioridade")
#define print_item(item) printf("%-30s %-20s %10d %10d\n", (item)->nome, (item)->tipo, (item)->quantidade, (item)->prioridade);

static void listar_itens(mochila_t *mochila)
{
  if (mochila->num_itens == 0)
  {
    fputs("Mochila vazia!", stderr);
    return;
  }

  print_item_header();
  for (int i = 0; i < mochila->num_itens; i++)
  {
    item_t *item = &mochila->itens[i];
    print_item(item);
  }
}

static void insertion_sort(mochila_t *mochila, criterio_ordenacao_e criterio)
{
  if (mochila->criterio_atual == criterio)
  {
    return;
  }

  int comparacoes = 0;

  switch (criterio)
  {
  case ORDENACAO_POR_NOME:
    for (int i = 1; i < mochila->num_itens; i++)
    {
      item_t item = mochila->itens[i];
      int j = i - 1;
      while (j >= 0)
      {
        comparacoes++;
        if (strcmp(mochila->itens[j].nome, item.nome) > 0)
        {
          mochila->itens[j + 1] = mochila->itens[j];
          j--;
        }
        else
        {
          break;
        }
      }
      mochila->itens[j + 1] = item;
    }
    mochila->criterio_atual = ORDENACAO_POR_NOME;
    break;

  case ORDENACAO_POR_TIPO:
    for (int i = 1; i < mochila->num_itens; i++)
    {
      item_t item = mochila->itens[i];
      int j = i - 1;
      while (j >= 0)
      {
        comparacoes++;
        if (strcmp(mochila->itens[j].tipo, item.tipo) > 0)
        {
          mochila->itens[j + 1] = mochila->itens[j];
          j--;
        }
        else
        {
          break;
        }
      }
      mochila->itens[j + 1] = item;
    }
    mochila->criterio_atual = ORDENACAO_POR_TIPO;
    break;

  case ORDENACAO_POR_PRIORIDADE:
    for (int i = 1; i < mochila->num_itens; i++)
    {
      item_t item = mochila->itens[i];
      int j = i - 1;
      while (j >= 0)
      {
        comparacoes++;
        if (mochila->itens[j].prioridade > item.prioridade)
        {
          mochila->itens[j + 1] = mochila->itens[j];
          j--;
        }
        else
        {
          break;
        }
      }
      mochila->itens[j + 1] = item;
    }
    mochila->criterio_atual = ORDENACAO_POR_PRIORIDADE;
    break;

  default:
    break;
  }
  printf("Comparações realizadas pelo insertion sort: %d\n", comparacoes);
}

static void ordenar_itens(mochila_t *mochila)
{
  if (mochila->num_itens < 2)
  {
    fputs("Não há itens suficientes para ordenar\n", stderr);
    return;
  }

  puts(MENU_ORDENACAO);
  int ord = 0;
  scanf("%d", &ord);
  criterio_ordenacao_e criterio = ORDENACAO_NAO_ORDENADO;
  switch (ord)
  {
  case 1:
    criterio = ORDENACAO_POR_NOME;
    break;

  case 2:
    criterio = ORDENACAO_POR_TIPO;
    break;

  case 3:
    criterio = ORDENACAO_POR_PRIORIDADE;
    break;

  default:
    fputs("Opção inválida\n", stderr);
    return;
  }

  insertion_sort(mochila, criterio);
}

static item_t *busca_binaria_por_nome(mochila_t *mochila, char const *nome)
{
  insertion_sort(mochila, ORDENACAO_POR_NOME);

  int inicio = 0;
  int fim = mochila->num_itens - 1;
  while (inicio <= fim)
  {
    int meio = (inicio + fim) / 2;

    int comp = strcmp(mochila->itens[meio].nome, nome);
    if (comp == 0)
      return &mochila->itens[meio];
    else if (comp < 0)
      inicio = meio + 1;
    else
      fim = meio - 1;
  }

  return NULL;
}

static void exibir_item(item_t *item)
{
  print_item_header();
  print_item(item);
}

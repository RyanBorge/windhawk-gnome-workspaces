# Windhawk GNOME Workspaces

Mod para o [Windhawk](https://windhawk.net/) no Windows 11 que replica o fluxo de áreas de trabalho virtuais (workspaces) do GNOME.

## Funcionalidades Planejadas

- **Workspaces Dinâmicos:** Mantém sempre `N+1` áreas de trabalho virtuais (com uma área vazia ao final) e remove áreas vazias intermediárias automaticamente.
- **Hot Corner:** Canto superior esquerdo ativo para abrir a Visão de Tarefas (*Task View*).
- **Scroll na Barra de Tarefas:** Alternância rápida entre áreas de trabalho virtuais rolando o scroll do mouse sobre a barra de tarefas.

## Requisitos e Compilação

- Windows 11
- Windhawk instalado
- Alvo de injeção: `explorer.exe`
- Flags do compilador: `-lole32 -loleaut32 -luuid`

## Instalação para Desenvolvimento

1. Abra o Windhawk e entre no modo de edição/criação de um novo mod.
2. Copie o conteúdo de `gnome-workspaces.wh.cpp` para o editor do Windhawk.
3. Compile e ative o mod.

# 🎨 Guia Completo de Símbolos Markdown Usados

## 1. Títulos

| Sintaxe | Resultado | Uso no README |
| :--- | :--- | :--- |
| `# Título` | Título H1 (maior) | Título principal (`# 📋 RACK MONITOR...`) |
| `## Título` | Título H2 | Seções principais (`## 🎯 Visão Geral`) |
| `### Título` | Título H3 | Subseções (`### Firmware`) |
| `#### Título` | Título H4 | Sub-subseções (`#### Arquitetura modular`) |
| `##### Título` | Título H5 | Nível mais baixo (raro) |
| `###### Título` | Título H6 (menor) | Nível mais baixo (raro) |

## 2. Ênfase

| Sintaxe | Resultado | Uso no README |
| :--- | :--- | :--- |
| `**texto**` | **texto** (negrito) | Destacar nomes de bibliotecas, conceitos |
| `*texto*` ou `_texto_` | *texto* (itálico) | Ênfase leve (pouco usado) |
| `` `texto` `` | `texto` (código inline) | Nomes de funções, variáveis, comandos |
| `~~texto~~` | ~~texto~~ (riscado) | Não usado no README |

## 3. Listas

| Sintaxe | Resultado | Uso no README |
| :--- | :--- | :--- |
| `- item` | Lista com bullets | Bibliotecas, comandos, checklist |
| `* item` | Lista com bullets (alternativo) | — |
| `1. item` | Lista numerada | Princípios de arquitetura |
| `  - item` (indentado) | Sublista | Regras de ouro (se necessário) |

## 4. Tabelas

| Sintaxe | Resultado |
| :--- | :--- |
| `\| Coluna 1 \| Coluna 2 \|` | Cabeçalho |
| `\| :--- \| :--- \|` | Alinhamento à esquerda |
| `\| :---: \| :---: \|` | Centralizado |
| `\| ---: \| ---: \|` | Alinhamento à direita |
| `\| dado \| dado \|` | Linha de dados |

**Exemplo de combinação de alinhamentos na mesma tabela:**

| Coluna Esquerda | Coluna Centro | Coluna Direita |
| :--- | :---: | ---: |
| texto | texto | texto |
| texto | texto | texto |

**Exemplo de tabela com múltiplas colunas:**

| A | B | C | D | E |
| :--- | :--- | :--- | :--- | :--- |
| 1 | 2 | 3 | 4 | 5 |

**Exemplo de tabela de hardware:**

| GPIO | Módulo | Função |
| :---: | :--- | :--- |
| 0 | LedRGB | Canal B |
| 2 | Audio | Buzzer D |
| 4 | LeitorRFID | RST |
| 5 | LeitorRFID | SS (SDA) |
| 13 | DHT22 | Dados (SDA) |
| 14 | Botao | Pushbutton |
| 15 | Audio | Buzzer E |
| 16 | Ultrassonico | ECHO |
| 17 | Ultrassonico | TRIG |
| 18 | LeitorRFID | SCK |
| 19 | LeitorRFID | MISO |
| 21 | DisplayOLED | I2C SDA |
| 22 | DisplayOLED | I2C SCL |
| 23 | LeitorRFID | MOSI |
| 25 | Porta | Switch |
| 26 | BotaoPanico | Pushbutton |
| 27 | PIR | Movimento |
| 32 | LedRGB | Canal R |
| 33 | LedRGB | Canal G |
| 34 | LDR | Leitura Analógica |

## 5. Links e Imagens

| Sintaxe | Resultado | Uso no README |
| :--- | :--- | :--- |
| `[texto](url)` | [texto](https://exemplo.com) (link clicável) | Referências, badges |
| `![alt](url)` | ![ESP32](https://img.shields.io/badge/ESP32-DevKit-blue) (imagem) | Badges (`![ESP32](https://...)`) |
| `[![alt](url)](link)` | [![ESP32](https://img.shields.io/badge/ESP32-DevKit-blue)](https://www.espressif.com/) (imagem clicável) | Badges com link |

## 6. Código

| Sintaxe | Resultado | Uso no README |
| :--- | :--- | :--- |
| `` `inline` `` | `inline` | Comandos, funções, nomes |
| ` ``` ` | Bloco de código | Trechos de shell, C++, JSON |
| ` ```cpp ` | Bloco com syntax highlight | Código C++ |
| ` ```bash ` | Bloco com syntax highlight | Comandos de terminal |
| ` ```ini ` | Bloco com syntax highlight | `platformio.ini` |
| ` ```json ` | Bloco com syntax highlight | Payloads JSON |
| ` ```text ` | Bloco sem highlight | Diagramas ASCII |
| ` ```cpp {1,3} ` | Destaca as linhas 1 e 3 | Código C++ (destaque) |
| ` ```cpp {1-3} ` | Destaca as linhas 1 a 3 | Código C++ (destaque) |

**Exemplo de bloco de código:**

```bash
pio test -e native
pio run -e esp32dev
```

## 7. Separadores

| Sintaxe | Resultado | Uso no README |
| :--- | :--- | :--- |
| `---` | Linha horizontal | Separar seções |
| `***` | Linha horizontal (alternativo) | — |
| `___` | Linha horizontal (alternativo) | — |
| `- - -` | Linha horizontal (alternativo) | — |

## 8. Citações e Destaques

| Sintaxe | Resultado | Uso no README |
| :--- | :--- | :--- |
| `> texto` | Citação (blockquote) | Avisos, notas importantes |
| `> **Nota:** texto` | Citação com negrito | Avisos destacados |

**Exemplo:**

```markdown
> **Nota:** o `main.cpp` é um orquestrador puro.
```

## 9. Emojis

Emojis são digitados diretamente como caracteres Unicode (sem sintaxe especial). No `README.md` usamos:

| Emoji | Nome | Uso |
| :---: | :--- | :--- |
| 📋 | Clipboard | Título principal |
| 🎯 | Alvo | Visão Geral |
| 📚 | Livros | Documentação |
| 📦 | Pacote | Arquitetura |
| 🧩 | Quebra-cabeça | Componentes |
| 🔧 | Chave inglesa | Hardware |
| 💾 | Disquete | Persistência NVS |
| 🖥️ | Monitor | Comandos Serial / Dashboard |
| 🧪 | Tubo de ensaio | Testes |
| 🎯 | Alvo | Princípios |
| ✅ | Check | O que está funcionando |
| 🚀 | Foguete | Próximos passos |
| 📌 | Tachinha | Lembretes |
| 📡 | Antena | MQTT |
| 🌐 | Globo | Dashboard |
| 🧠 | Cérebro | Regras de ouro |
| ⚠️ | Aviso | Limitações |
| ❌ | X vermelho | O que não funciona |
| ☑ | Checkbox marcado | Checklist |
| ☐ | Checkbox vazio | Checklist (não usado) |
| → | Seta direita | Fluxos |
| — | Travessão | Separar nome/descrição |
| ✅ / ❌ | Check/X | Status (Retain) |

## 10. Checklist

| Sintaxe | Resultado | Uso no README |
| :--- | :--- | :--- |
| `- [ ] item` | ☐ item (não marcado) | Não usado |
| `- [x] item` | ☑ item (marcado) | Não usado (usamos `- ☑ item`) |
| `- ☑ item` | ☑ item | Usado em "O que está funcionando" |

> **Diferença:** `- [x]` é o checklist **interativo** do GitHub (você pode clicar). `- ☑` é apenas o caractere Unicode (não interativo). Como o README é documentação, usamos o Unicode.

## 11. Badges (Shields.io)

| Sintaxe | Resultado |
| :--- | :--- |
| `[![alt](url_imagem)](url_link)` | Badge clicável |
| `![alt](url_imagem)` | Badge estático |

**Exemplo do README:**

```markdown
[![ESP32](https://img.shields.io/badge/ESP32-DevKit-blue?logo=espressif&logoColor=white)](https://www.espressif.com/)
```

### 12. Links internos (âncoras)

| Sintaxe | Resultado |
| :--- | :--- |
| `[texto](#secao)` | Link para seção da mesma página |
| `[texto](./arquivo.md)` | Link para outro arquivo |

**Exemplo:**

```markdown
[Clique aqui para acessar a documentação](./docs/html/index.html)
```

### 13. Quebras de linha

| Sintaxe | Resultado |
| :--- | :--- |
| `linha 1` + Enter + `linha 2` | Duas linhas no mesmo parágrafo |
| `linha 1` + Enter + Enter + `linha 2` | Dois parágrafos separados |
| `linha 1` + `  ` (dois espaços) + Enter + `linha 2` | Quebra de linha forçada (`<br>`) |

## 14. Escapes de caracteres especiais

| Sintaxe | Resultado |
| :--- | :--- |
| `\|` | \| (barra vertical escapada) |
| `\*` | \* (asterisco escapado) |
| `\_` | \_ (underscore escapado) |
| `\#` | \# (cerquilha escapada) |
| `\\` | \\ (barra invertida escapada) |
| `` \` `` | \` (crase escapada) |

## 15. HTML inline

Markdown aceita HTML, o que permite recursos extras:

| Sintaxe | Resultado |
| :--- | :--- |
| `<br>` | Quebra de linha |
| `<b>texto</b>` | <b>texto</b> (negrito via HTML) |
| `<i>texto</i>` | <i>texto</i> (itálico via HTML) |
| `<sub>texto</sub>` | <sub>texto</sub> (subscrito) |
| `<sup>texto</sup>` | <sup>texto</sup> (sobrescrito) |
| `<kbd>Ctrl</kbd>` | <kbd>Ctrl</kbd> (tecla) |
| `<mark>texto</mark>` | <mark>texto</mark> (destacado) |
| `<details><summary>Título</summary>Conteúdo</details>` | Bloco recolhível |

**Exemplo de bloco recolhível:**

<details>
<summary>Clique para expandir</summary>

Este é o conteúdo que fica oculto até você clicar.

</details>

## 16. Notas de rodapé

| Sintaxe | Resultado |
| :--- | :--- |
| `[^1]` | Nota de rodapé (referência) |
| `[^1]: Texto da nota` | Definição da nota |

**Exemplo:**

```markdown
Esta é uma frase com nota de rodapé.[^1]

[^1]: Esta é a nota de rodapé.
```

## 17. Tabela de conteúdo (TOC)

| Sintaxe | Resultado |
| :--- | :--- |
| `- [Título](#título)` | Link interno (manual) |
| `<!-- toc -->` | Marcador de TOC automático (alguns renderizadores) |

**Exemplo de TOC manual:**

```markdown
- [1. Títulos](#1-títulos)
- [2. Ênfase](#2-ênfase)
- [3. Listas](#3-listas)
```

## 18. Tabela resumo: o que usamos no README

| Categoria | Sintaxe | Onde |
| :--- | :--- | :--- |
| Título principal | `#` | Topo |
| Seções | `##` | Maioria das seções |
| Subseções | `###` | Firmware, Arquitetura |
| Negrito | `**texto**` | Nomes de bibliotecas, conceitos |
| Código inline | `` `texto` `` | Funções, comandos, variáveis |
| Bloco de código | ` ``` ` | Shell, C++, JSON |
| Lista com bullets | `- item` | Bibliotecas, comandos |
| Lista numerada | `1. item` | Princípios de arquitetura |
| Tabela | `\| a \| b \|` | Hardware, MQTT, comandos |
| Link | `[texto](url)` | Referências |
| Imagem | `![alt](url)` | Badges |
| Citação | `>` | Notas, avisos |
| Separador | `---` | Entre seções |
| Checklist Unicode | `- ☑ item` | O que está funcionando |
| Emojis | Caractere Unicode | Títulos das seções |
| Travessão | `—` | Separar nome/descrição |

## ✅ Resumo visual

O que você vê no README renderizado:

- **Títulos grandes** (`#`, `##`, `###`)
- **Texto em negrito** (`**texto**`) para destaque
- **Texto em código** (`` `texto` ``) para comandos e variáveis
- **Tabelas** para organização visual
- **Listas** com bullets (`-`) e numeradas (`1.`)
- **Links e badges** para navegação rápida
- **Emojis** para identificação visual instantânea
- **Separadores** (`---`) para dividir seções
- **HTML inline** (<br>, <kbd>, <mark>, <details>) para recursos extras

Tudo isso é **Markdown padrão** e funciona no GitHub, GitLab, VS Code Preview e qualquer renderizador compatível.

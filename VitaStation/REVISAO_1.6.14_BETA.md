# VitaStation 1.6.14 beta — categoria Vita e personalização

Comparação: [1.6.13 → 1.6.14](https://github.com/CarlosTuga/VitaStation/compare/v1.6.13-beta...v1.6.14-beta). Pacote nativo 01.74. Bridge/componentes continuam 1.6.12; esta alteração não exige novo plugin ou reinício.

## Categoria Vita

A ordem no topo passa a Jogos, **Vita**, Homebrew, Recentes, Favoritos, Loja, PSP / PS1. L/R e toque seguem a mesma ordem, mantendo os identificadores internos das categorias anteriores para compatibilidade com a base existente.

Vita inclui as aplicações de utilização direta da consola que estavam na entrada de definições: Música, Vídeos, Fotografias, Troféus, Navegador e Gestor de Conteúdo, apenas quando estão presentes. Exclui serviços internos com nomes semelhantes. Usa os seus ícones/metadados e o mesmo arranque com X confirmado na 1.6.13. Uma cache pequena separada evita pesquisar todos os SFO do sistema a cada arranque; a base principal existente continua utilizável.

Em **Definições → Definições do VitaStation → Aplicações da categoria Vita**, aparece uma lista alfabética com caixas de seleção. X adiciona/remove aplicações adicionais; as seis aplicações de origem estão identificadas como fixas. Este menu gere a categoria, não lança aplicações. A seleção manual só é alterada nas definições, não na categoria principal. Adicionar aqui não remove a aplicação da sua categoria habitual. A escolha fica em `ux0:data/VitaStation/vita-category.txt`, com ficheiro temporário/geração anterior para recuperação. É aplicada após carregar a cache e nas reconstruções. O menu usa IDs estáveis: se a biblioteca for reconstruída enquanto está aberto, não passa a editar outra aplicação por alteração dos índices. Recentes mantém ordenação por utilização; as restantes categorias mantêm data de instalação.

## Personalização

Novo menu **Definições → Definições do VitaStation → Personalização: temas e imagens**:

- **10 esquemas visuais simples**, baseados em cores, gradientes e padrões procedurais: PS5 Midnight, Ice, Cosmic, Aurora; PS4 Blue Wave, Sunset, Ocean; Aniversário Classic, Platinum, Legacy. Somam-se aos cinco esquemas de cores anteriores. A lista tem pré-visualização; só X aplica e guarda. Os painéis acompanham o tema e os fundos próprios surgem quando não há imagem de jogo ou wallpaper escolhido.
- **Fonte**: original do sistema, Rajdhani Medium, Audiowide Regular e ficheiros TTF pessoais na pasta `fonts`. As duas fontes adicionais e as licenças SIL OFL estão incluídas no VPK. Se uma fonte guardada faltar ou falhar ao carregar, permanece a fonte do sistema. As etiquetas do topo ajustam a escala à largura disponível.
- **Wallpaper**: PNG/JPG/JPEG das pastas `wallpapers` ou `ux0:picture`, com procura até dois níveis de subpastas e lista limitada a 128 entradas. O wallpaper escolhido substitui visualmente o fundo dos jogos; escolher Automático repõe os fundos de jogo/tema. Há limites de ficheiro/dimensões no carregador. A cache evita descodificar o mesmo fundo em cada frame.
- **Bloqueio real da Vita**: a opção exporta a imagem escolhida para a biblioteca nativa de Fotografias usando ScePhotoExport; a opção seguinte abre Fotografias com a rota de lançamento corrigida. **Ainda é necessário abrir a imagem em Fotografias e escolher “Usar como ecrã inicial”.** Guardar uma preferência no VitaStation não é apresentado como alteração automática do bloqueio. Não escreve caminhos de sistema não verificados nem modifica a base de temas da consola. Esta etapa nativa necessita de validação física.

Pastas para os teus ficheiros (criadas ao abrir o seletor):

```text
ux0:data/VitaStation/wallpapers/
ux0:data/VitaStation/lockscreens/
ux0:data/VitaStation/fonts/
```

Preferências de fonte/imagens em `ux0:data/VitaStation/appearance.ini`; tema e cores continuam no `config.ini`. As gravações preservam a geração anterior. Os ficheiros pessoais permanecem na memória entre arranques. As fontes do pacote permanecem em `app0:assets/fonts/`.

## Preservado

Não altera a rota de abertura direta validada na 1.6.13, o áudio confirmado, a pesquisa a cada dois arranques, a reconstrução manual, plugins ou clocks. Mantém o pacote de carátulas 1.04 e os seus caminhos. O registo de compilação passa a usar a versão real em `version.h`; anteriormente a mensagem de diagnóstico permanecia fixada em 1.6.11 apesar de o pacote ter sido atualizado.

## Validação e limites

Passaram os testes host/integração, o adaptador de abertura nativo de produção e testes novos da categoria/persistência, compatibilidade com a cache existente, navegação, adição/remoção apenas nas definições, aplicações fixas, linhas vazias por toque, reconstrução sem troca de identidade, dez temas distintos e recuperação de uma gravação interrompida. As pré-visualizações do renderer de host foram inspecionadas para verificar a disposição dos novos menus. Compilação e validação VitaSDK/VPK concluídas.

Após testar a 1.6.14, o utilizador informou que aparentemente tudo está a funcionar bem e pediu para manter esta versão. Indicou que os temas são visualmente simples, sobretudo cores, e aceitou deixá-los assim por agora. Esse relato confirma o funcionamento geral observado, sem constituir verificação individual de todas as opções. Não é alegada aplicação automática do bloqueio: mantém-se a etapa manual em Fotografias descrita acima.

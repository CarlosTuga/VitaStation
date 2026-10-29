# VitaStation 1.6.13 beta — recuperar a abertura direta das aplicações

## Resultado confirmado e comparação

Em 29 de outubro de 2026, o utilizador confirmou na PS Vita que o VitaStation voltou a abrir aplicações corretamente, incluindo Need for Speed Most Wanted com um único X, após fechar e voltar a abrir o launcher. Esta confirmação valida a recuperação da abertura observada; não é uma certificação de todas as aplicações, integrações ou diálogos do sistema.

Comparação com a release pública anterior: [1.6.9 → 1.6.13](https://github.com/CarlosTuga/VitaStation/compare/v1.6.9-beta...v1.6.13-beta).

Além da abertura direta descrita abaixo, inclui as correções dos candidatos intermédios:

- Áudio: repõe a prioridade compatível da thread, conserva o PCM pré-carregado e restaura/reabre a porta dos efeitos após o boot quando necessário. Os sons foram confirmados pelo utilizador na 1.6.10 e preservados nesta versão.
- Saída e arranque: pesquisa da biblioteca não bloqueia a abertura; preparação que já escreve plugins termina em segurança. O próprio serviço grava os marcadores de conclusão e a cache, sem depender de mais uma passagem da interface.
- Preparação: componentes atuais usam verificações de leitura e não recalculam todo o backup sem necessidade de alterações; o runtime PSP evita verificações redundantes. Mantém as melhorias de velocidade confirmadas pelo utilizador e a pesquisa a cada dois arranques.

## Regressão e diagnóstico

O utilizador confirmou que a abertura funcionava em versões anteriores e deixou de funcionar após as alterações ao encerramento/abertura através do Shell. Nas tentativas mais recentes, o plugin 1.6.12 confirmou pedidos para VitaShell e um jogo, devolveu zero de `LaunchAppByUri`, mas a sonda cinco segundos depois não encontrou os destinos em execução. A aceitação do pedido não provou a execução. O log do launcher ainda identificava o executável instalado como 1.6.11; o plugin ativo era efetivamente 1.6.12. Nenhuma dessas tentativas valida a abertura física como corrigida.

A comparação com a 1.6.7 mostrou que existia uma rota direta do próprio launcher que foi removida a favor de um handoff obrigatório. Este candidato retira essa dependência para a biblioteca, aplicações de sistema e Apollo que usam `launcher_launch_app`.

## Alterações concretas

- `src/launcher.c` guarda o destino localmente após validar instalação/ID. Não escreve pedidos ao Shell nem espera um ACK para aceitar a seleção.
- `src/main.c` só executa a abertura na fronteira de saída, depois de terminar uma preparação de plugins já em curso e guardar as definições. A pesquisa de biblioteca continua sem bloquear a saída.
- `launcher_commit_exit` cancela pedidos/ACKs antigos, chama a URI `psgm:play?titleid=...` com flags de compatibilidade `0xFFFFF`, e main termina o processo imediatamente após aceitação. Segue a sequência observável em [VitaShell, launchAppByUriExit](https://github.com/TheOfficialFloW/VitaShell/blob/master/utils.c#L393). Não copia código do projeto.
- Retorno negativo da API mantém o VitaStation aberto e apresenta o erro. O destino é consumido uma única vez, sem tentativas duplicadas do plugin. `launcher-direct` regista o retorno real da API, separadamente da seleção inicialmente em fila.

A biblioteca continua a abrir com um só X, sem novo menu da aplicação. A sequência pretende permitir ao sistema substituir o launcher; não é uma implementação de interceção global ou de pressão simulada no botão OK de diálogos do Shell. A abertura visível foi confirmada pelo utilizador. A ausência do diálogo em todos os percursos do sistema não foi confirmada separadamente.

## Preservado e instalação

Áudio funcional confirmado pelo utilizador, PCM e prioridades; velocidade de arranque; cache e pesquisa a cada dois arranques; reconstrução manual; preparação e backup de plugins/configuração; carátulas 1.04. Não altera clocks, energia ou configurações de outros plugins.

Versão 1.6.13 beta, APP_VER 01.73. O bridge empacotado continua 1.6.12, sem mudanças. Instalar o VPK no VitaShell e abrir o VitaStation; esta alteração de abertura está no executável, não requer carregar outro plugin nem novo reinício se 1.6.12 já estiver ativo.

## Validação e estado

Passaram os testes host e de integração, incluindo IDs inválidos, retorno negativo sem encerramento, retorno positivo, URI e flags de compatibilidade. `tests/run_native_launch.py` compila o adaptador Vita de produção com SDK simulado e verifica seleção sem chamada/ACK do Shell, execução apenas no commit de saída, cancelamento de pedidos antigos, ausência de duplicação e manutenção do processo perante erro. Compilação e validação VitaSDK/VPK concluídas.

A abertura real foi confirmada pelo utilizador na consola, incluindo Need for Speed Most Wanted com um único X. Sons e velocidade também foram confirmados durante estes testes. Apollo, catálogo AutoPlugin, atualização/desinstalação e ausência do diálogo em todos os percursos não foram revalidados fisicamente nesta confirmação e mantêm os respetivos limites documentados. Os candidatos 1.6.10–1.6.12 não foram publicados como correções confirmadas.

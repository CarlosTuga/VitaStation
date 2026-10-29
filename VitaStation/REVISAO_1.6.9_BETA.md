# VitaStation 1.6.9 beta — áudio, Apollo, AutoPlugin e abertura

Comparação: [1.6.8 → 1.6.9](https://github.com/CarlosTuga/VitaStation/compare/v1.6.8-beta...v1.6.9-beta).

## Som de arranque com ruído

A leitura FTP confirmou que o PCM instalado era igual ao ficheiro distribuído. Foi alterado o transporte, não substituído o áudio por outro som: `audio_clip.c` carrega o clip completo em memória alinhada e completa o último bloco com zeros. `audio.c` usa blocos de 2048 amostras, sem ler o cartão a cada 256 amostras e sem reutilizar um buffer que possa ainda estar a ser consumido. Espera pelo fim da saída antes de libertar a memória, reduz o volume da porta durante o arranque e restaura a configuração dos efeitos depois. A thread de áudio tem prioridade maior, sem alterar frequências/energia da consola.

`audio_preview.inc` passa a alternar dois buffers de descodificação e drena a porta antes de libertar os dados. Mantém o som PS5 incluído na 1.6.8, com origem e atribuição em assets/audio/. A audição real desta correção ainda precisa de teste na consola; não é declarada qualidade sonora comprovada fisicamente.

## AutoPlugin II sem lista e Apollo sem abrir

Antes, os pedidos dependiam do mesmo serviço que carátulas, atualização e pesquisas de jogos. `store_vita.cpp` passa a ter um serviço dedicado às integrações. Em `integration_requests.inc`, abrir o catálogo tenta primeiro carregar os 140 registos locais, sem aguardar a fila de rede nem o serviço de carátulas. Triângulo continua a consultar a lista remota; obter/instalar um plugin usa o serviço dedicado. Operações incompatíveis continuam serializadas. Erros de serviço e pedidos ocupados passam a ter estado explícito e registo, em vez de retornar silenciosamente.

Se Apollo já estiver instalado como NP0APOLLO, o pedido de abertura é preparado diretamente, sem primeiro esperar por pesquisas/importação de metadados. Se faltar, a descarga/instalação verificada continua no serviço dedicado. Não é introduzido um motor Apollo interno nem são alterados saves automaticamente. A instalação física de plugins/Apollo e a sua compatibilidade continuam por validar.

## Aviso de encerramento

O utilizador confirmou que apenas reabriu o launcher depois da 1.6.8; o plugin novo ainda não tinha sido carregado por um reinício completo. Também confirmou que EXIT_TO não suprimiu o aviso no seu teste. A 1.6.9 retira esse caminho: `launcher.c` só aceita uma passagem confirmada ao plugin. O VitaStation guarda os dados/termina a preparação em curso e sai; o plugin aguarda o desaparecimento efetivo do processo antes de lançar o destino. Não chama a abertura comum nem EXIT_TO enquanto VitaStation continua ativo. Não é um clique automático indiscriminado em OK, nem um diálogo personalizado.

`VitaStationBoot/main.c` mantém o pedido confirmado até 180 segundos para permitir terminar escritas em curso, cancelando se o pedido for retirado ou expirar. O nonce inicial mantém o limite de cinco segundos. Evita perder o destino enquanto o launcher termina uma preparação. Um marcador de instalação abandonado só é retirado quando o auxiliar instalador já não está a correr. O registo `bridge-version.txt` passa a ser tentado na thread até a memória estar acessível, em vez de apenas uma vez durante o carregamento do módulo.

**É necessário instalar a 1.6.9 e reiniciar completamente a consola.** Suspender ou reabrir o VitaStation não carrega um plugin substituído. O registo deve então indicar 1.6.9. Se o plugin não confirmar o pedido, a abertura falha com erro real, em vez de voltar à via que provoca o quadrado Cancelar/OK. A ausência visual do aviso requer teste físico. Aberturas feitas diretamente pelo SceShell/LiveArea não atravessam esta função; não é alegada uma interceção global dos diálogos do sistema.

## Mantido da 1.6.8

Contagem das 21.811 carátulas, instalação direta com progresso, backup apenas dos plugins sem copiar a aplicação, atualização automática, áudio local dos jogos compatíveis e pesquisa da biblioteca a cada dois arranques da consola. Sem alteração de frequências, voltagens ou perfis de energia.

## Testes e limites

- Testes do clip PCM: alinhamento, preservação das amostras entre blocos, padding silencioso, ficheiro ausente/inválido e libertação.
- Testes do código real dos pedidos de integração: catálogo local independente da fila, sinalização do serviço dedicado, Apollo instalado direto, serviço ausente e instalação ativa.
- Testes da abertura: pedido confirmado exige saída do chamador; sem confirmação não usa abertura nativa/EXIT_TO; IDs inválidas rejeitadas.
- Testes do nonce inicial e validade estendida após confirmação, atalhos PS/L e limites de remoção; regressões host, integração e menus aprovadas.
- Compilação e validação do VPK pelo VitaSDK aprovadas. Áudio ouvido na Vita, abertura após reinício, execução do Apollo e instalação de plugins reais ainda pendentes.

As cópias privadas de diagnóstico da consola não são publicadas. As notas distinguem alterações de código de resultados físicos ainda não observados.

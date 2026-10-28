# VitaStation 1.3 — 28/10/2026

## Abertura das aplicações

Foi encontrada uma falha concreta em launcher.c: o validador terminava com false mesmo depois de validar os nove caracteres do Title ID. Consequentemente, a API nunca era chamada. Foi corrigido e separado o validador do backend de plataforma. A Vita verifica a presença do SFO e pede sceAppMgrLaunchAppByUri(0x20000, "psgm:play?titleid=..."). Essa chamada/flag corresponde ao header instalado e à documentação oficial: https://docs.vitasdk.org/group__SceAppMgrUser.html

O resultado distingue ID inválido, aplicação ausente, plataforma indisponível e rejeição pelo sistema. O log ux0:data/VitaStation/diagnostic.log inclui Title ID, estado e código hexadecimal devolvido. Aceitar um pedido não prova que o jogo arrancou; o teste final é na consola. Em host não se simula sucesso de lançamento de uma aplicação Vita. Os testes injetam um backend exclusivamente para provar que IDs válidos chegam à chamada, com flags/URI corretos, e que falhas se propagam.

## Visual

Carrossel superior mais compacto, espaços calculados pelas dimensões animadas dos cartões, ações circulares para favoritos/detalhes, cabeçalho com atalhos funcionais, destaques claros e Centro de Controlo refinado. O fundo preserva a proporção da imagem, recortando o excesso em vez de a esticar. Os elementos são desenhados nativamente; não há uma imagem estática do sistema PS5.

## Loja e definições

O seletor reabria sempre na opção zero, sem pré-selecionar a aplicação persistida. Agora recupera o índice através do ID guardado. As definições mostram o nome real da aplicação. O ficheiro config.ini é novamente lido após a gravação e comparado com o conteúdo esperado antes de indicar sucesso. No ramo Vita usam-se sceIoRename/sceIoRemove, com erros registados. Preserva-se um backup válido se a geração principal estiver corrompida; não se afirma atomicidade ou resistência garantida a perda de energia.

A escolha só é invalidada quando existe uma biblioteca validada e a aplicação está ausente. Uma falha de acesso ao diretório não limpa a preferência. Erros de gravação da loja conservam a escolha anterior; erros das preferências de som/animação revertem a alteração e mostram mensagem. Formatos antigos continuam legíveis.

Existe uma quinta categoria Loja, depois de Favoritos. Mostra a aplicação instalada escolhida como destino, nome/ícone/arte disponíveis e alguns ícones reais da biblioteca. A apresentação é inspirada numa loja de consola; não inventa produtos, preços, promoções ou catálogos. Abrir loja chama o mesmo lançador real dos jogos. Alterar loja abre o seletor na aplicação atual; ambas as ações podem ser usadas por D-pad e X ou por toque. As outras quatro categorias mantêm a ordem anterior.

## Volume

Barra temporária no topo da VitaStation, atualizada pelo valor real 0–30 lido por sceAVConfigGetSystemVol, mostrado em percentagem. Deteta mudanças por leitura a cada 80 ms e eventos de teclas quando a plataforma os disponibiliza. Não altera a resposta nativa de volume nem simula níveis. Falhas de leitura são registadas e não criam uma barra fictícia. Como os bits de volume podem ser filtrados fora de SceShell, uma tecla pressionada num limite em que o valor não muda pode não renovar a barra se não houver evento exposto. Esta barra é da VitaStation; não foi criada uma sobreposição global para outros jogos.

## Logótipo de abertura

Ecrã nativo com monograma VS, título “Vita Station” e texto menor “desenvolvido por Carlos Tuga”. Aparece antes da descoberta da biblioteca e permanece aproximadamente 1,8 segundos após a descoberta, com desvanecimento final. X permite avançar sem lançar uma aplicação por acidente. O tempo é real, não uma contagem fixa de frames. Se o scanner bloquear, o primeiro ecrã pode manter-se mais tempo.

## Botão PS global — pendente

Pedido registado: um toque abre a VitaStation; dois toques regressam ao LiveArea original. O utilizador confirmou firmware 3.65 com HENkaku/Ensō. Isto não está ativo no VPK: uma aplicação normal não recebe globalmente os eventos físicos PS quando outro jogo está em execução. O valor SCE_CTRL_PSBUTTON nas leituras normais também pode indicar interceção, e não é seguro tratá-lo como um evento global de tecla.

Foi investigada integração em SceShell e referências de plugins públicos. Continua pendente uma implementação e validação do encaminhamento em SceShell, incluindo jogos suspensos, long press e prevenção de ciclos de abertura. Não se incluiu um plugin que apenas parecesse cumprir o pedido nem se alterou tai/config.txt automaticamente. Fontes primárias consultadas: headers VitaSDK instalados e https://github.com/hotdog-face/VitaOS/blob/main/plugin/vitaos_ps.c (a implementação consultada não resolve diretamente todo o gesto global pedido).

## Verificação

Resultados efetivamente executados constam de TEST_RESULTS.txt, BUILD_VITA_RESULTS.txt e INSTALL_FIX_RESULTS.txt. A suite inclui validador/caminho de lançamento; propagação de resultado; roundtrip repetido de definições; reabertura do seletor no destino guardado; falha de biblioteca sem perda da loja; remoção após biblioteca validada; ação de abrir loja ligada ao lançador; cinco categorias; volume inicial, mudanças, limites, erro e expiração; tempo/skip do ecrã inicial; e regressões anteriores. O backend injetado e o renderer host não substituem testes na Vita.

As capturas previews são fixtures TESTE, produzidas pelas chamadas reais da UI e rasterizadas em host. Não são capturas da consola nem um catálogo incluído no produto. O utilizador confirmou instalação/abertura e controlos de revisões anteriores; abertura real de aplicações, persistência e volume desta revisão ainda precisam de confirmação no equipamento.

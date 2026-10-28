# Relatório final — VitaStation 1.1

27 de outubro de 2026. Foram implementadas e verificadas em host as fontes desta revisão. Atualização: compilação ARM e geração de VPK concluídas com o VitaSDK em `D:\Vita sdk` (ver BUILD_VITASDK.md). O funcionamento na consola e os objetivos de desempenho permanecem por validar; a especificação completa não deve ser considerada certificada como concluída.

## A — Arquitetura

`main.c` coordena ciclo, arranque e encerramento; `ui.c` gere estados e navegação; `render.c` contém vita2d e captura opcional de comandos; `library.c` contém scanner, SFO e vistas; `settings.c` gere persistência; `texture.c` contém cache; `audio.c` possui a thread Vita e `audio_queue.c` a FIFO partilhada/testável; `launcher.c` pede lançamentos; `log.c` regista diagnóstico. Headers em `include/`, ícone em `assets/`, testes e captura em `tests/`.

Preservou-se a base de renderização/controlos/sons fornecida e refatoraram-se os componentes incorretos. As estruturas grandes do ciclo principal são estáticas. O scanner prepara uma biblioteca temporária antes de publicar um resultado bem-sucedido.

## B — Funcionalidades

Implementadas nas fontes: quatro categorias; carrossel; seleção e animações; fundos/crossfade; detalhes; menu rápido; definições; destino de loja; favoritos; histórico; classificação manual; persistência; scanner; validação SFO; controlos físicos/touch; efeitos sonoros; logs e tratamento de falhas. No ramo Vita, Jogar chama a API real. Em host, o lançamento retorna falha.

Não existem controlos fictícios para música de fundo, brilho ou volume do sistema. O controlo de áudio liga/desliga os efeitos da VitaStation.

## C — Interface e recursos

O PSD fornecido foi aberto e inspecionado: composição conceptual 3840 × 2160 com home, menu e detalhes. O arquivo opcional de ecrãs 960 × 544 não foi fornecido. A UI desenha os próprios textos, cartões, seleção, botões e painéis; adota carrossel inferior, tons azuis, sombreado e fundo em destaque. Não apresenta o mockup como imagem estática.

Arte instalada: `sce_sys/icon0.png` e `pic0.png`; sem arte usa fallback procedural. Há ícone próprio e SVG de referência em `assets/`. `previews/` contém sete diagnósticos produzidos pelas chamadas reais de `ui_render`, rasterizados com Pillow/Segoe UI no PC. Os nomes TESTE são fixtures exclusivas do capturador, não são biblioteca fictícia no executável Vita. Fonte PGF e GPU podem produzir diferenças. Foram inspecionados home, detalhes, menu, definições, loja, carregamento e vazio; corrigiram-se sobreposição no menu e cortes de texto.

## D — Biblioteca e SFO

O parser lê até 1 MiB, descodifica little endian, verifica magic/version, contagem, separação das tabelas, offsets e tamanhos utilizados/máximos, terminação de chaves/strings e operações de ficheiro. Valida todas as entradas e evita somas de offsets não confiáveis. Aceita tipos SFO string `0x0204`, binário `0x0004` e inteiro `0x0404`.

O scanner exige diretório com ID alfanumérico de nove caracteres e SFO válido. TITLE_ID, quando disponível, deve coincidir com o diretório. Nome ausente usa o ID; SFO inválido é ignorado e registado. Exclui `VTST00001`, deduplica, preserva dados das aplicações presentes e remove entradas desaparecidas. O limite atual é 256 aplicações. `gd` identifica aplicação Vita, não prova que seja comercial.

## E — Homebrew

HOMEBREW é uma vista própria. Não foi estabelecido um indicador SFO universal de homebrew: aplicações ambíguas continuam acessíveis em JOGOS, e Detalhes permite classificação manual persistente. Não há classificação por nome nem promessa de reconhecimento automático de todos os homebrews.

## F — Recentes

Ordenação decrescente por timestamp, apenas para aplicações atualmente existentes. O histórico cobre pedidos feitos pela VitaStation, não sessões externas ou duração de jogo. O contador é apresentado como pedidos de lançamento. Guarda-se antes da API porque o sistema pode terminar o launcher; em erro devolvido pela API, contador/data são revertidos e novamente guardados. Não há garantia de rollback se o processo terminar nesse intervalo.

## G — Favoritos

Persistem por ID; entradas removidas deixam de aparecer como instaladas sem afetar os restantes favoritos. Falha ao guardar um favorito é comunicada. Metadados de aplicações removidas não são conservados indefinidamente no novo ficheiro da biblioteca.

## H — Loja e lançamento

A seleção manual percorre aplicações instaladas, com nome e ícone disponíveis, sem assumir que todo homebrew é loja. Guarda/limpa o destino e invalida uma aplicação que desapareceu. LOJA tenta lançar o destino; sem escolha, pede configuração. A ligação automática à Store oficial está pendente: a opção predefinida deixa o destino vazio.

Corrigiu-se `sceAppMgrLaunchAppByUri` de `0xFFFFF` para `0x20000`, conforme o [header oficial do VitaSDK](https://github.com/vitasdk/vita-headers/blob/master/include/psp2/appmgr.h), mantendo `psgm:play?titleid=...`. Verifica ID e existência do SFO antes da chamada, e regista erros da API. Retorno não negativo significa pedido aceite, não prova execução do jogo. Não há substituição de ficheiros da Store.

## I — Persistência

Configuração e CSV em `ux0:data/VitaStation/`. A nova geração tem cabeçalho de versão e marcador final. Escreve `.tmp` binário; verifica escrita, flush e fecho; reabre e valida o marcador; roda o principal para `.bak`; promove o temporário; tenta restaurar o backup se a promoção falhar. A leitura tenta principal e depois backup; não confia em temporários não promovidos. Mantém leitura do formato antigo.

Não há afirmação de atomicidade ou durabilidade perante perda de energia. Marcadores detetam uma nova geração incompleta, mas não substituem checksum. O formato antigo tem validação menos forte. A compatibilidade do rename/armazenamento Vita exige teste real.

## J — Áudio, memória e threads

UI produtora e thread Vita consumidora; FIFO de 16 posições protegida por mutex e acordada por semáforo. Fila cheia rejeita o pedido novo, preservando a ordem anterior. Inicialização falhada liberta recursos; encerramento assinala paragem e espera pela thread antes de destruir objetos. PCM sintetizado; sem música de fundo.

Texturas têm pins para fundo atual, alvo e frame em desenho. LRU só remove entradas sem pins. Após terminar o desenho Vita espera-se o GPU antes de retirar pins do frame; isso privilegia segurança e pode afetar desempenho. Cache normal: 32 MiB estimados de pixels, 64 slots; PNG até 1920 × 1088. Esse orçamento não inclui todos os custos internos de decode/GPU. Cache de falhas evita tentativas repetidas imediatas e é limpo ao atualizar a biblioteca. O delta time de animação é real e limitado a 100 ms.

## K — Testes efetivamente executados

Windows, Python 3.13 e Zig 0.16.0. Comando: `python tests/run_host.py --zig`. Compilação C99 com `-Wall -Wextra -Werror`, sem avisos/erros nessa execução. Os sete grupos da suite terminaram com código 0; saída em `TEST_RESULTS.txt`.

Exercitaram-se: biblioteca vazia/inexistente; SFO válido/inválido e autoexclusão; rescan e preservação; remoção real de SFO; deduplicação; recentes ordenados; favoritos; classificação; persistência ausente/inválida; roundtrip; backup; falha de gravação por pasta inexistente; recuperação de geração incompleta; loja selecionada/guardada/removida; navegação e toque nas categorias; menus/definições/pedido de atualização; orçamento e slots de cache; dimensões excessivas; pinning/evicção; crossfade entre dois handles e alterações rápidas; fila áudio, ordem, capacidade, mute e paragem; IDs e falha de lançamento host.

O fixture SFO de 49 bytes foi truncado em cada comprimento 0–48; seis campos de offsets/tamanhos receberam `UINT32_MAX`; realizou-se uma mutação por byte. Isto não equivale a fuzzing extensivo. Os fundos de crossfade são buffers de teste: não verificam o descodificador PNG Vita.

Também se compilou o conjunto com `main.c` para smoke host e o capturador de layout. Atualização posterior: CMake/Ninja, compilação ARM, ligação VitaSDK e criação do VPK foram executados com sucesso. Continuam sem execução: sanitizers, Valgrind, CTest, stress concorrente na consola ou medições de FPS.

## L — Problemas encontrados e corrigidos

Parser sem limites completos; SFO inválido aceite como aplicação; evicção de fundos e texturas já referenciadas pelo frame; pedido de áudio sobrescrito; apenas Games/Media; flag de lançamento incorreta; sucesso fictício em host; delta fixo; remoção antecipada da configuração válida no ramo Vita; texto cortado/sobreposto. O README original afirmava compilação e testes que não foram comprovados nesta sessão: essas afirmações foram removidas.

## M — Pendências reais

Scanner e decode PNG continuam síncronos: existe ecrã de descoberta, mas a UI não avança durante a operação. Falta carregamento assíncrono se as medições o justificarem. Falta integração automática da Store oficial. O packaging foi executado e a integridade ZIP do VPK foi verificada; instalação na consola permanece pendente. Não se testaram PNGs corrompidos no descodificador Vita, perda de energia real, nem ausência de fugas/data races com instrumentação. Não se afirma cumprimento de 60 FPS ou equivalência visual final ao PSD.

## N — Limitações e validação na consola

No teu SDK: configurar/compilar, verificar dependências e stubs, criar VPK e confirmar ícone. Na Vita: arranque/saída, biblioteca real, jogos/homebrew, controlos/touch, falhas de lançamento, loja, persistência, áudio, crossfade sob pressão de cache e sessões prolongadas. Medir RAM, decode e tempo por frame antes de afirmar fluidez.

Esta revisão não implementa boot, plugins, substituição de LiveArea, alterações Ensō/taiHEN, downloads, instalações automáticas ou remoção de aplicações oficiais. Os ZIPs originais foram preservados.

## Correcao de instalacao — 28/10/2026

Apos o erro 0x8010113D comunicado na consola, verificou-se que icon0.png era RGBA (tipo PNG 6). Foi convertido para PNG de paleta, 8 bits (tipo 3), 128x128 e sem entrelacamento. O VPK foi novamente gerado pelo SDK. O script de build passou a verificar os recursos PNG e os CRCs. O validador rejeitou o pacote RGBA anterior e aceitou o novo pacote. Falta confirmar a instalacao na consola. Referencia: https://github.com/TheOfficialFloW/VitaShell/issues/312

## Atualização 1.3

A revisão mais recente e as pendências atuais estão documentadas em REVISAO_1.3.md. Inclui correção do validador de lançamento, recuperação e verificação da loja guardada, quinta categoria Loja, HUD de volume local e assinatura de abertura. O encaminhamento global do botão PS permanece pendente.

VitaStation 1.6.0 beta

Biblioteca PSP e PS1
- Nova categoria PSP / PS1, com navegação L/R e toque. Pesquisa ux0:app, ux0:pspemu/PSP/GAME e ux0:pspemu/ISO. Também pesquisa os caminhos pspemu de uma0 e imc0.
- Lê bolhas PSP/PS1 registadas no LiveArea através da base ur0:shell/db/app.db, aberta apenas para leitura; o registo do sistema nunca é editado diretamente. Corrige a verificação de instalação para as bolhas PSP que não têm pasta em ux0:app. A pesquisa contempla bolhas com metadados LiveArea e conteúdos em PSP/GAME.
- Identifica bolhas Adrenaline com boot.bin e jogos ISO PSP e EBOOT.PBP PSP/PS1. Extrai título, DISC_ID e ícone dos metadados ISO9660/PBP; as carátulas locais são alternativa quando disponíveis. Deduplica caminhos e prefere a bolha nativa existente para o mesmo conteúdo. ISO de PS1 em BIN/CUE e imagens comprimidas CSO/DAX/ZSO não têm leitor nesta versão; PS1 é suportado em EBOOT.PBP. A capacidade atual da biblioteca continua limitada a 256 entradas.
- X num ISO/PBP sem bolha prepara e instala um pequeno atalho próprio e pede a execução pelo AppMgr. O ISO não é convertido para um executável Vita: a bolha referencia o ficheiro original e usa Adrenaline. Os dados do jogo não são movidos. O ID estável mantém favoritos/histórico quando a bolha é criada.

Componentes incluídos e primeira abertura
- Inclui o conjunto compatível de cinco módulos Adrenaline/AdrBubbleBooter/bootconv e um carregador de bolhas, extraídos do payload RetroFlow v8.5.0, com créditos, licenças, hashes e fonte pública correspondente incluídos.
- Na primeira reconstrução prepara o conjunto automaticamente apenas para o Adrenaline 7 original reconhecido por CRC dos três módulos. Esta versão foi confirmada na consola do utilizador. Versões desconhecidas ou conjuntos misturados são preservados e recusados; não são substituídos silenciosamente.
- Guarda cópias de segurança em ux0:data/VitaStation/psp-runtime-backup e ficheiros .vst-previous. Valida todos os payloads e grava um marcador de reinício antes de publicar os novos módulos; restaura os anteriores se uma renomeação falhar. O diagnóstico e os testes de recuperação não constituem garantia contra corte de energia durante uma atualização. É necessário reiniciar a consola depois da primeira preparação para recarregar o kernel. Não inclui firmware Sony; o Adrenaline tem de estar instalado e inicializado.
- Os módulos são preparados ao abrir VitaStation, não pelo simples ato de instalar o VPK no VitaShell. Nenhuma instalação de módulos foi executada remotamente neste trabalho. As bolhas são criadas a pedido de abertura, não em massa durante a pesquisa.

Instalação e atualização da base de dados
- Os formatos suportados descarregados pela loja mantêm a instalação automática. Contador de conclusões e marcador persistente assinalam instalações locais mesmo quando outra fase da fila substitui o estado visível.
- Observa alterações nas pastas de aplicações/LiveArea/PSP/ISO e reconstrói a base depois de as alterações estabilizarem. Se o launcher estiver fechado, verifica a geração na abertura seguinte. Não executa uma pesquisa completa nos arranques normais sem alterações; mantém o intervalo de 10 aberturas e o botão manual.
- PKG em fila do LiveArea continua a cargo do serviço do sistema. VPK/ZIP locais continuam a exigir que o launcher permaneça aberto até terminar. Nem todo ficheiro arbitrário é uma aplicação instalável.

Ordenação e lançamento
- Jogos, Homebrews, PSP/PS1 e Favoritos apresentam primeiro as instalações mais recentes. Usa a data do LiveArea quando disponível e a criação do ficheiro como alternativa. Datas desconhecidas ficam no fim; empates são ordenados pelo título/ID. Recentes continua exclusivamente ordenada pela última utilização.
- O adaptador nativo tenta LaunchAppByName com LIVEAREA_OFF/DEBUG_CALLBACK/EXIT_TO (0x70000), mantendo URI 0x20000 como alternativa. Guarda o histórico antes do pedido e fecha o próprio VitaStation após aceitação; não destrói outras aplicações nem injeta um OK em diálogos globais. Se uma preparação estiver ativa, guarda o pedido e executa-o depois, sem exigir novo X. A ausência de confirmação do sistema ainda exige teste físico.
- Solicita uma vez o arranque automático e o retorno PS pelo componente VitaStationBoot quando este está configurado; mantém a opção para o utilizador desativar depois. LiveArea continua a ser o shell do sistema.

Personalização e legibilidade
- Cinco cores de ambiente e cinco tons de letras, guardados nas preferências, com alto contraste ativado por predefinição. Texto com opacidade reforçada e sombra discreta; texto escuro dos botões claros é preservado. Bandeiras e imagens mantêm as suas cores.
- Todas as opções de definições têm ícone semântico antes do nome e painel com ícone, nome, descrição e valor/estado. Inclui rede, conta, troféus, dispositivos, áudio, ecrã, relógio, armazenamento, energia, loja, descargas, plugins, base de dados, cores e texto. Os ícones são desenhados pelo programa e não precisam de rede.

Auditoria das definições da consola
- As 78 entradas foram revistas: 2 controlos nativos de volume/brilho, 12 consultas apenas de leitura, 1 ação de suspensão e 63 entradas ainda sem adaptador. Não se afirma que estas 63 opções foram implementadas. PSN, avatar, sincronização de troféus, emparelhamento Bluetooth e edição/scan de rede continuam indisponíveis.
- Ao reconstruir, o diagnóstico consulta os controlos/leitores disponíveis sem alterar valores e grava ux0:data/VitaStation/console-settings-audit.tsv. Distingue resposta confirmada, leitura falhada, apenas leitura e ação não executada. Mapa de código em mapping/AUDITORIA_CONSOLA_1.6.0.csv.

Validação
Testes host e integração, ISO/PBP (incluindo PS1), ordenação por instalação/uso, cache, cores e preferências aprovados. Testes do código real de preparação PSP com chamadas da consola simuladas verificam versão, backups, rollback, marcador de reinício, SFO/caminho de bolha, chamada à geração de cabeçalho e promoção simulada e rejeição de colisões. Teste do adaptador AppMgr verifica flags e alternativa URI. Compilação VitaSDK e validação SFO/ícone/VPK aprovadas. A abertura de jogos, a atualização efetiva de módulos, o comportamento do Shell e a aparência final não foram testados fisicamente nesta versão.

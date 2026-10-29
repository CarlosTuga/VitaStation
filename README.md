> **1.6.10 beta:** repõe a prioridade compatível do áudio, abre destinos pela API do Shell e impede gestos PS antigos de interferirem na troca. Mantém o arranque rápido. Instalar e reiniciar completamente para carregar o plugin 1.6.10. [Instruções, funções e limites atuais](VitaStation/REVISAO_1.6.10_BETA.md). A revisão desta versão prevalece sobre descrições históricas abaixo. Teste físico pendente.

# VitaStation

**Launcher para PlayStation Vita — desenvolvido por Carlos Tuga.**

VitaStation organiza e abre jogos, aplicações e homebrews numa interface inspirada na PS5. Integra biblioteca PSP/PS1, loja por catálogos locais, instalação de formatos suportados, carátulas permanentes e personalização visual.

**Versão do launcher: 1.6.10 beta. Instalador de carátulas: 1.04.**

Este projeto é independente e não é um produto oficial Sony. A beta não substitui integralmente o sistema da Vita. “Implementado” descreve o código disponível; testes de PC e compilação não equivalem a validação física de todas as funções.

## Estado atual

| Área | Estado |
| --- | --- |
| Interface, organização, preferências e cores | Implementadas e testadas ao nível do código |
| Biblioteca Vita e homebrew | Pesquisa e base de dados persistente implementadas |
| PSP/PS1 | Leitura de bolhas LiveArea, ISO de PSP e PBP implementada |
| Criação de bolhas e preparação Adrenaline | Implementadas; funcionamento real desta versão ainda por validar na consola |
| Loja e instalação | PKG pelo serviço LiveArea; VPK/ZIP suportados pelo instalador local |
| Carátulas | Leitura local permanente; instalação por VPK separado |
| Definições da consola | 2 controlos, 12 consultas, 1 ação e 63 entradas sem adaptador |
| PS, Power e arranque automático | Ponte incluída; dependem de configuração, reinício e comportamento do sistema |
| Avisos do Shell | Supressão total não garantida; não existe injeção automática de OK em diálogos globais |

Ver [falhas e limitações](docs/FALHAS_E_LIMITACOES.md), [testes](docs/TESTES.md) e [componentes](docs/PLUGINS.md).

## Organização deste pacote

- **VitaStation/**: código, recursos, componentes incluídos, ferramentas e testes do launcher.
- **Caratulas-de-loja/**: código, manifesto, inventário, créditos e ferramentas do instalador de imagens.
- **docs/**: documentação de utilização, compilação, validação e publicação.
- **release-assets/**: VPKs e arquivo de fontes limpas para anexar a GitHub Releases. Os binários desta pasta estão excluídos do Git normal.
- **tools/**: verificador da preparação pública.
- **LICENSING.md** e **COPYING-GPL-3.0.txt**: âmbito das licenças e texto GPL dos componentes abrangidos.

Backups pessoais da consola, bases de dados reais, configurações taiHEN pessoais, IP/FTP, recibos de transferência e temporários não integram a publicação. Os ficheiros originais permanecem na pasta de trabalho.

## Requisitos

- PlayStation Vita com ambiente homebrew compatível. O desenvolvimento teve como alvo firmware 3.65 com desbloqueio permanente.
- Instalador VPK, como VitaShell, e permissões necessárias para homebrew inseguro.
- Para executar os ISO/PBP através das novas bolhas: **Adrenaline 7 original instalado, inicializado e funcional**. Firmware Sony não é incluído.
- Ligação à Internet para as descargas e requisitos próprios de cada conteúdo. PSM necessita do respetivo runtime instalado; um plugin não o substitui.
- Espaço para VPKs, instalação, ficheiros temporários, carátulas e cópias de segurança. O tamanho comprimido não representa todo o espaço necessário durante a operação.

Não é garantida compatibilidade com todas as versões, modelos, emuladores e configurações de plugins.

## Instalação

1. Obtém os ficheiros **VitaStation-1.6.0-beta.vpk** e **Caratulas-de-loja-1.04.vpk** na Release. Nesta preparação local estão em release-assets/.
2. Instala o VPK do launcher pelo VitaShell.
3. Abre o VitaStation e aguarda a primeira pesquisa e preparação dos componentes. Instalar o VPK no VitaShell, por si só, **não executa** essa preparação.
4. Reinicia quando indicado para carregar os módulos taiHEN/Adrenaline preparados. Os ficheiros anteriores são preservados em cópias de segurança.
5. Instala o VPK de carátulas, abre **Carátulas de loja** no LiveArea, prime **X** e espera pela conclusão da cópia em bloco.
6. Fecha e volta a abrir o VitaStation.

Se o pacote de carátulas 1.04 já estiver totalmente copiado, atualizar apenas o launcher não exige copiá-lo novamente. Termina ou cancela uma cópia/instalação antes de atualizar a aplicação que a está a executar.

## Categorias e ordenação

| Categoria | Conteúdo e ordem |
| --- | --- |
| Jogos | Jogos Vita identificados pelos catálogos locais; instalações mais recentes primeiro |
| Homebrews | Outras aplicações e conteúdos não identificados como jogos Vita; instalações mais recentes primeiro |
| Recentes | Aplicações abertas, ordenadas pela **última utilização** |
| Favoritos | Itens marcados pelo utilizador; instalações mais recentes primeiro |
| Loja | Catálogos integrados e escolha da loja externa predefinida |
| PSP / PS1 | Bolhas e ficheiros compatíveis PSP/PS1; instalações mais recentes primeiro |

A classificação por catálogo é uma correspondência de metadados, não uma certificação de autoria ou licença. Existem ajustes manuais. A biblioteca tem atualmente um limite de **256 entradas**.

A data de instalação usa o registo LiveArea quando disponível e a criação do ficheiro como alternativa. Datas desconhecidas ficam no fim; empates são resolvidos por título/ID. Favoritos e histórico são guardados separadamente da base reconstruível.

### Pesquisa de aplicações e ficheiros

- **ux0:app/**: aplicações e bolhas Vita.
- **ur0:shell/db/app.db**: registo LiveArea, aberto **apenas para leitura** na pesquisa PSP/PS1.
- **ux0:pspemu/PSP/GAME/**: jogos em PBP.
- **ux0:pspemu/ISO/**: ISO de PSP.
- Caminhos pspemu equivalentes em **uma0:** e **imc0:**.

O scanner lê títulos, IDs, categorias e ícones compatíveis, deduplica caminhos e prefere bolhas nativas conhecidas para o mesmo conteúdo. Ícones extraídos são guardados em ux0:data/VitaStation/media-icons/.

## Base de dados e arranque rápido

A base validada fica em ux0:data/VitaStation/library-cache.bin. Nos arranques normais, o programa reutiliza os metadados e carrega os ícones visíveis a pedido, sem repetir a pesquisa completa em segundo plano.

A reconstrução ocorre quando:

- A base está ausente, incompatível ou danificada.
- Se atingem **10 aberturas do VitaStation** desde a última reconstrução bem-sucedida.
- O utilizador escolhe **Definições → Definições do VitaStation → Reconstruir base de dados**.
- Termina uma instalação local ou são detetadas alterações nas pastas observadas.
- O utilizador regressa ao launcher e existem alterações feitas enquanto estava fechado.

O intervalo conta aberturas da aplicação, não reinícios físicos. Uma reconstrução necessária pode decorrer em segundo plano. A primeira abertura com uma estrutura de base diferente pode demorar mais. Um pedido X feito durante a preparação fica pendente e é retomado no fim.

## Execução direta e PSP/PS1

**X abre diretamente a aplicação selecionada.** O botão de informação no ecrã tátil mantém o acesso aos detalhes, favoritos e classificação.

O adaptador tenta lançamento por nome com LIVEAREA_OFF/DEBUG_CALLBACK/EXIT_TO e mantém a rota URI validada como alternativa. Guarda o histórico antes do pedido e fecha o próprio VitaStation após aceitação. Aceitação não confirma arranque completo do jogo, nem elimina necessariamente avisos do Shell sobre outras aplicações.

### Bolhas existentes

As bolhas PSP/PS1 que não têm uma pasta normal em ux0:app são procuradas no registo do LiveArea e nos conteúdos PSP/GAME. São reconhecidos também metadados de bolhas Adrenaline com boot.bin suportado.

### ISO/PBP sem bolha

Ao premir X num ficheiro compatível, o launcher pode preparar um atalho próprio com título e caminho do jogo, instalar essa bolha e pedir a execução. **O ISO não é convertido para um jogo nativo de PS Vita:** a bolha referencia o ficheiro original e usa o Adrenaline partilhado. O jogo não é movido.

A criação é feita a pedido, não em massa durante a pesquisa. O ID estável permite conservar favoritos/histórico na passagem para a bolha.

Formatos lidos nesta versão: **ISO de PSP e EBOOT.PBP de PSP/PS1**. BIN/CUE de PS1 e imagens comprimidas CSO/DAX/ZSO não têm leitor implementado.

## Loja integrada

A navegação e apresentação dos catálogos decorrem dentro do VitaStation, sem abrir Usagi PKGj ou PS Vita Alive apenas para mostrar os itens.

- TSV separado por tipo; JSON como alternativa quando o TSV correspondente não existe.
- Jogos Vita, DLC, temas, PSP, DLC PSP, PS1, PSM e Homebrew.
- Demonstrações desativadas.
- Catálogos de jogos/conteúdos filtrados para **Europa e EUA**, com bandeiras junto ao título e nos detalhes. Japão, Ásia e regiões desconhecidas são excluídos deste filtro.
- Homebrew usa a integração **PS Vita Alive**, com metadados, requisitos e ligações suportadas, sem o mesmo filtro regional.
- Título, ID, região, tamanho quando fornecido, carátula e ações de descarga.
- Escolha persistente de uma aplicação instalada como loja externa predefinida.

Pastas de catálogos: ux0:usagi-pkgj/, ur0:usagi-pkgj/, ux0:pkgj/ e ur0:pkgj/. Exemplos: titles_psvgames.tsv, titles_psvdlcs.tsv, titles_psxgames.tsv e titles_psmgames.tsv. O catálogo Homebrew base acompanha o projeto.

### Descargas e instalação automática

| Tipo | Comportamento |
| --- | --- |
| PKG compatível | Encaminhado para o serviço de descargas LiveArea, com requisitos verificados por tipo |
| VPK Homebrew | Descarga, verificação e instalação locais; o launcher deve permanecer aberto |
| ZIP/dados/plugin Homebrew suportado | Mecanismo local correspondente; não instala qualquer arquivo arbitrário |
| Ligação informativa/formato não suportado | Não é apresentada como instalação concluída |

Uma tarefa aceite no LiveArea não equivale a uma instalação terminada. O progresso/cancelamento das tarefas do sistema pertence ao LiveArea. A biblioteca é atualizada após as conclusões/alterações observáveis ou na abertura seguinte, caso o launcher esteja fechado.

**Instalar DLCs com o jogo** procura DLCs da mesma ID exata. Catálogo, requisitos e limite de tarefas podem impedir a instalação de todos os DLCs. Não se afirma compatibilidade com todos os instaladores Homebrew. A abertura de aplicações é bloqueada durante uma instalação local ativa para evitar interrompê-la.

## Carátulas permanentes

Destino partilhado pelo launcher e instalador:

    ux0:data/VitaStation/covers/covers.bin
    ux0:data/VitaStation/covers/manifest.tsv

O pacote 1.04 tem **21 811 imagens/aliases** num bloco. O índice local permite localizar cada imagem; tamanho, CRC e limites de imagem são verificados. Existem alternativas para ficheiros individuais e caches suportados de versões anteriores.

Reiniciar a consola não elimina as imagens. Depois da cópia, remover a bolha do instalador não elimina a pasta externa. Eliminação manual, formatação ou restauro podem removê-las.

Cobertura do pacote para os IDs Europa/EUA do catálogo usado:

- **PSM: 413/413 IDs**.
- **PS1: 374/379 IDs**.
- Cinco coletâneas PS1 sem imagem confirmada; não receberam carátulas de outros produtos.

Estes números não garantem cobertura de futuros catálogos. Existem também quatro ícones Homebrew indisponíveis nas fontes consultadas. Fontes e relatórios acompanham Caratulas-de-loja/.

A cópia em bloco evita criar mais de vinte mil ficheiros individuais. O tempo depende do cartão; uma cópia física da versão 1.03 anterior demorou cerca de 9 minutos, sem constituir promessa para a 1.04. Cancelar faz a cópia em bloco recomeçar do início; o bloco final anterior é preservado até à publicação do novo.

Esta beta privilegia imagens locais, sem pedidos automáticos HTTPS de carátulas. A atualização automática do catálogo Homebrew também está desativada nesse modo; existem operações manuais suportadas.

## Definições e personalização

### Definições do VitaStation

Som, loja externa, animações, voltar ao início, arranque com a consola, plugins, PS, energia, DLCs automáticos, cores de ambiente, cores de texto e alto contraste. Inclui biblioteca/execução, instalação de pacotes, descargas/fila, carátulas/metadados, navegação L/R/favoritos, preferências, logótipo/arranque, reconstrução da base e diagnóstico/recuperação taiHEN.

**Gestão de aplicações e dados** corresponde à reconstrução da base do launcher; não é um gestor completo de ficheiros/desinstalação da consola.

Há **cinco ambientes e cinco tons de letras**, guardados entre arranques. Alto contraste reforça a opacidade e acrescenta sombra às letras claras, preservando texto escuro nos botões claros. Imagens e bandeiras mantêm as suas cores.

O arranque apresenta a marca VitaStation e o crédito «desenvolvido por Carlos Tuga». A interface inclui animações opcionais e efeitos sonoros sintetizados para navegação/confirmação/menu, que podem ser desativados. Não é distribuído um pacote de sons oficiais da PS5.

Cada opção tem um ícone semântico antes do nome e um painel com ícone, nome, descrição e estado/valor. Os ícones não dependem da rede. O regressa primeiro às categorias; nesse ecrã, volta à biblioteca.

### Definições da consola

Das **78 entradas**, existem **2 controlos** de volume/brilho, **12 consultas apenas de leitura**, **1 ação de suspensão** e **63 entradas sem adaptador**. As consultas não são controlos editáveis.

Perfil/avatar PSN, estado online, sincronização de troféus, edição/procura de redes Wi-Fi, modo de avião e emparelhamento Bluetooth continuam **indisponíveis**.

O diagnóstico guarda ux0:data/VitaStation/console-settings-audit.tsv com resultados das leituras disponíveis, sem alterar valores nem executar ações de energia. Os mapas completos estão em VitaStation/mapping/DEFINICOES_1.6.0.csv e AUDITORIA_CONSOLA_1.6.0.csv.

## Plugins incluídos e preparação automática

Preparados ao abrir/reconstruir ou pelas opções correspondentes. **Configurado não significa carregado na sessão atual**: módulos de kernel normalmente exigem reinício.

| Componente | Ativação/destino | Função e condição |
| --- | --- | --- |
| VitaStationBoot.suprx | Pasta VitaStation no taiHEN ativo; *main | Ponte para arranque, gestos PS e pedido Power; marcadores de ativação |
| nonpdrm.skprx | *KERNEL | Compatibilidade dos conteúdos Vita correspondentes |
| nopsmdrm.skprx | *KERNEL | Compatibilidade PSM; runtime PSM continua necessário |
| NoPspEmuDrm_kern.skprx | *KERNEL | Caminhos PSP/PS1 e encaminhamento LiveArea correspondentes |
| NoPspEmuDrm_user.suprx | *ALL | Componente de utilizador correspondente |
| npdrm_free_mod.prx | pspemu/seplugins; game.txt/vsh.txt | Alternativa PSP; preserva npdrm_free.prx existente |
| adrenaline_kernel.skprx | PSPEMUCFW/sce_module | Parte do conjunto de arranque de bolhas |
| adrenaline_user.suprx | Mesmo conjunto | Componente de utilizador compatível |
| adrenaline_vsh.suprx | Mesmo conjunto | Componente VSH compatível |
| adrbubblebooter.suprx | Mesmo conjunto | Encaminhamento para o conteúdo escolhido |
| bootconv.suprx | Mesmo conjunto | Compatibilidade da configuração de bolhas |

VitaStationBoot é gerado do código incluído. A preparação usa o taiHEN ativo, valida hashes, evita duplicados, preserva escolhas explicitamente desativadas e cria cópias ao alterar ficheiros. Conflitos com outros launchers/variantes podem impedir a operação.

O conjunto Adrenaline é preparado **apenas para o Adrenaline 7 original reconhecido pelos módulos**. Versões desconhecidas/misturadas são preservadas e recusadas. Backups ficam em ux0:data/VitaStation/psp-runtime-backup/ e ficheiros de recuperação. Não se garante recuperação automática de um corte de energia durante a publicação dos módulos.

SQLite, AppUtil, PAF e Promoter são módulos do próprio sistema carregados quando necessários, não firmware distribuído. vdb_daemon.suprx/NeoVitaDB não integra as dependências automáticas deste pacote. CapUnlocker, ttbr_remap, azaharnative e ioplus são ligações opcionais de determinados itens Homebrew, não plugins instalados indiscriminadamente.

Versões, secções, hashes, origens e fontes: [PLUGINS.md](docs/PLUGINS.md), VitaStation/assets/plugins/, VitaStation/assets/psp-runtime/ e VitaStation/third_party/plugin-sources/.

## Arranque, PS, Power e controlos

A ponte pretende abrir o launcher quando o Shell estiver pronto, encaminhar um toque PS para o VitaStation e duplo toque para LiveArea, e pedir o menu **Desligar / Reiniciar / Cancelar** ao manter Power. A captura global e a interpretação das rotas ainda exigem validação física.

Uma solicitação inicial de arranque/retorno PS é feita quando a ponte está configurada; o utilizador pode desativar depois. SELECT permite contornar o arranque opcional. SELECT + L + R contorna o encaminhamento PS nessa sessão. O HUD observa volume/botões quando expostos à aplicação.

O SceShell não é removido: LiveArea continua a ser o shell do sistema. Não existe confirmação automática de todos os diálogos globais.

| Controlo | Função |
| --- | --- |
| L/R | Categorias e navegação correspondente nas definições |
| Setas/analógico e toque | Seleção/navegação |
| X | Abrir, confirmar ou encaminhar descarga/instalação |
| O | Voltar/cancelar; na biblioteca pode fechar o launcher |
| Quadrado | Favorito na biblioteca; atualização na loja |
| Triângulo | Definições na biblioteca; informação na loja |
| START | Centro de controlo/menu rápido |
| SELECT | Cancelamento das operações locais suportadas; não interrompe uma transação de instalação a meio |

## Dados persistentes

Em ux0:data/VitaStation/: config.ini guarda preferências; userdata.csv guarda favoritos/histórico/classificação; library-cache.bin é a base reconstruível; library-starts.txt conta aberturas; installation-state.txt/library-dirty assinalam alterações; covers/ contém imagens; media-icons/ contém ícones extraídos; bgdl/ contém auxiliares da fila; psp-runtime-backup/ guarda módulos anteriores; diagnostic.log e console-settings-audit.tsv fornecem diagnósticos.

O gestor de plugins protege entradas essenciais de arranque/armazenamento. Configurações, registos e backups pessoais não devem acompanhar o repositório público.

## Falhas e limitações conhecidas

1. As 63 definições sem adaptador continuam indisponíveis; não existe integração completa de PSN, troféus, rede e Bluetooth.
2. Os novos caminhos PSP/PS1, atualização real de módulos, captura PS/Power e comportamento final do Shell precisam de validação física desta beta.
3. Nem todos os avisos do sistema são suprimidos. Pedido de lançamento aceite não prova arranque completo.
4. VPK/ZIP locais precisam do launcher aberto. Fila LiveArea não prova instalação concluída.
5. Existem carátulas/ícones ausentes, incluindo cinco coletâneas PS1 e quatro ícones Homebrew nas fontes consultadas.
6. As falhas HTTPS foram mitigadas com um conjunto de raízes menor e verificação mantida. Usar carátulas offline não confirma todos os downloads HTTPS de aplicações.
7. Espaço, cartão lento, interrupções e backups conservados afetam a cópia de imagens e atualizações.
8. Formatos/instaladores não são universais; a preparação automática Adrenaline depende da versão reconhecida.
9. A biblioteca está limitada a 256 entradas e datas antigas nem sempre são recuperáveis.

Ver [FALHAS_E_LIMITACOES.md](docs/FALHAS_E_LIMITACOES.md) para detalhes, incluindo erros históricos e distinção entre falha confirmada e comportamento ainda não validado.

## Compilação e testes

O SDK não acompanha o repositório. Ver [COMPILAR.md](docs/COMPILAR.md). Em Windows, na raiz desta pasta:

    python -m pip install cmake ninja
    powershell -NoProfile -ExecutionPolicy Bypass -File .\VitaStation\build-vita.ps1 -SdkPath 'D:\Vita sdk'

Saída: VitaStation/build-vita/VitaStation.vpk. Não está declarado um build bit a bit reproduzível com qualquer snapshot do SDK.

Testes principais:

    python VitaStation/tests/run_host.py --zig
    python VitaStation/tests/run_integration.py
    python VitaStation/tests/run_media.py
    python VitaStation/tests/run_launch_handoff.py
    python VitaStation/tests/run_cover_pack.py
    python Caratulas-de-loja/tests/run_tests.py

Zig requer o pacote ziglang. O teste de preparação de bolhas recebe o VPK RetroFlow v8.5.0 como fixture; ver o documento de compilação. Não altera uma consola.

## Publicação no GitHub

Os fontes estão preparados nesta pasta. Os VPKs e o arquivo de fontes limpas ficam localmente em release-assets/, para anexar à Release; essa pasta exclui binários do Git normal.

O GitHub bloqueia ficheiros acima de 100 MiB no Git normal. O VPK de carátulas tem cerca de 269 MB: usa **GitHub Releases**, ou Git LFS se for esse o fluxo escolhido. Ver [documentação oficial](https://docs.github.com/en/repositories/working-with-files/managing-large-files/about-large-files-on-github) e [PUBLICACAO_GITHUB.md](docs/PUBLICACAO_GITHUB.md).

Repositório do projeto: https://github.com/CarlosTuga/VitaStation. Os VPKs são distribuídos separadamente do histórico Git. Os binários devem acompanhar as fontes correspondentes e licenças. Verificação local: python tools/verify_publication.py.

## Licenças e créditos

Ver [LICENSING.md](LICENSING.md), [THIRD_PARTY.md](VitaStation/THIRD_PARTY.md) e as licenças incluídas. O conjunto contém componentes MIT/BSD/GPL/zlib e recursos com termos próprios. **Não é um projeto integralmente MIT.** Código derivado de VitaShell e módulos abrangidos por GPL exigem cumprir os termos correspondentes e fornecer as fontes.

As carátulas HexFlow têm os termos da coleção, incluindo restrição não comercial. PS Vita Alive tem termos próprios e exceções. Os titulares mantêm os direitos sobre imagens e marcas. O logótipo PlayStation não é uma marca própria do VitaStation nem uma licença concedida pela Sony.

Créditos principais: Carlos Tuga; TheOfficialFloW/TheFloW; Usagi PKGj e colaboradores; PS Vita Alive; HexFlow; RetroFlow; AdrBubbleBooter/LMAN, ONElua e contribuidores; VitaSDK e restantes dependências. Os avisos detalhados estão preservados nos componentes.

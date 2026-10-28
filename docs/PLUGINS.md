# Plugins, módulos e ativação

Referência: VitaStation 1.6.0 beta. A preparação ocorre ao abrir/reconstruir o launcher ou pelas opções correspondentes; não é um script executado pelo VitaShell durante a simples instalação do VPK.

## Ponte própria

VitaStationBoot.suprx é compilado de VitaStation/plugins/VitaStationBoot/ e incluído no VPK. A secção taiHEN é *main; o destino é a pasta VitaStation dentro do taiHEN ativo (ux0:tai quando essa configuração existe; caso contrário ur0:tai).

Usa os marcadores de arranque, PS e Power em ux0:data/VitaStation/. O arranque espera pelo Shell; SELECT contorna-o. PS simples/duplo e Power prolongado usam as rotas de sistema descritas no código, com comportamento físico ainda por validar. SELECT+L+R contorna o encaminhamento PS da sessão. Não substitui o SceShell.

## Plugins taiHEN incluídos

| Ficheiro | Versão | Secção | SHA-256 |
| --- | --- | --- | --- |
| nonpdrm.skprx | v1.2 | *KERNEL | 6600602020e8aedd0ec4bc55941f0349f63f06ebe6ef91f390e1e0a15e19f3b0 |
| nopsmdrm.skprx | v1.5 | *KERNEL | 0a78b44d3c78264647c34632c20c14741539c988d4244012872d93c804cc5cd1 |
| NoPspEmuDrm_kern.skprx | v1.06 | *KERNEL | 70e1c8bcfffd7d25e0af2f5d47fb6f15ad2e1d271f73a43e64fa94387198e470 |
| NoPspEmuDrm_user.suprx | v1.06 | *ALL | 261fb224ea24b7bc3f9dfca71de0da90bbdeff3e39c3cba31a517b8403ef788f |

Origens e commits:

- nonpdrm.skprx: https://github.com/TheOfficialFloW/NoNpDrm, commit ef073fd7c8f773e6d31a1634b9bf06d54b268d08.
- nopsmdrm.skprx: https://github.com/frangarcj/NoPsmDrm, commit ce80bee441e945c13c701342b2bcd28822d94c70.
- NoPspEmuDrm_kern.skprx: https://github.com/LiEnby/NoPspEmuDrm, commit c1ba02adbd890b48f3c6535265ba7d9b3301c156.
- NoPspEmuDrm_user.suprx: https://github.com/LiEnby/NoPspEmuDrm, commit c1ba02adbd890b48f3c6535265ba7d9b3301c156.

NoNpDrm aplica-se aos caminhos Vita correspondentes, NoPsmDrm aos conteúdos PSM e NoPspEmuDrm ao emulador PSP/caminhos LiveArea. São dependências de compatibilidade específicas, não uma garantia de que qualquer conteúdo instala/executa. PSM continua a necessitar do runtime.

O preparador valida os binários incluídos, evita duplicados, preserva entradas explicitamente desativadas e cria backups ao alterar a configuração. Uma versão já configurada noutro caminho pode ser preservada; não se afirma que todos os plugins da consola sejam substituídos. Variantes/conflitos conhecidos podem impedir a operação. Uma alteração de secção *KERNEL exige normalmente reinício da consola. A lista do gestor é uma leitura da configuração; presença de ficheiro/entrada não prova módulo carregado na sessão.

## Plugin PSP

npdrm_free_mod.prx, v7.3; origem https://github.com/lusid1/npdrm_free_mod; commit 8830df952c0dc90b9ff463c95754a7dd06532396; SHA-256 5a1bc15c6af3f8aa53fbe86ed4bde23abe66faccea10ea88290bd1d3dc53fb82.

É preparado em ux0:pspemu/seplugins e nas entradas game.txt/vsh.txt. Se já existe npdrm_free.prx, esse ficheiro é preservado e usado. Mudanças PSP podem exigir reiniciar o Adrenaline. As fontes e a licença estão em third_party/plugin-sources.

## Conjunto de bolhas Adrenaline

Cinco módulos compatíveis e um carregador genérico foram obtidos dos payloads RetroFlow v8.5.0. Os módulos são adrenaline_kernel.skprx, adrenaline_user.suprx, adrenaline_vsh.suprx, adrbubblebooter.suprx e bootconv.suprx. O destino é ux0:app/PSPEMUCFW/sce_module/; não são cinco entradas taiHEN independentes criadas indiscriminadamente. O carregador e a instalação partilhada fazem o encaminhamento necessário.

Só é preparada automaticamente a instalação Adrenaline 7 original reconhecida pelo conjunto de CRCs: kernel 20f7c5cd, user 5487a9c3 e vsh fa85cd74. Conjuntos desconhecidos/misturados são recusados. Estes identificadores não substituem um teste físico de todas as combinações carregadas em memória.

O preparador valida os módulos, guarda os originais em psp-runtime-backup e ficheiros .vst-previous, usa staging e marcador de reinício antes da publicação, e tenta rollback em falha de renomeação. Após a primeira preparação é necessário reiniciar. Corte de energia durante a publicação pode exigir recuperação manual. Nenhum firmware Sony é incluído.

A criação de uma bolha é a pedido de abertura do ISO/PBP, preserva o ficheiro de jogo e verifica colisões de ID. Usa o gerador de cabeçalho e Promoter do projeto. Os caminhos foram testados com chamadas de consola simuladas; execução real ainda por validar.

Hashes individuais, origem de release, créditos e licenças: VitaStation/assets/psp-runtime/ORIGINS.json e os ficheiros da mesma pasta. A fonte pública AdrBubbleBooter-oss está em third_party/plugin-sources/AdrBubbleBooter-oss-source.zip.

## Outros componentes

SQLite, AppUtil, PAF e Promoter são módulos do sistema carregados por API, não payloads de firmware distribuídos. vdb_daemon.suprx/NeoVitaDB não é uma dependência automática incluída.

CapUnlocker.skprx, ttbr_remap.skprx, azaharnative.skprx e ioplus.skprx fazem parte de ligações opcionais validadas de determinados itens Homebrew; não são instalados automaticamente para todos os utilizadores. Os backups particulares de plugins da consola não acompanham esta publicação.

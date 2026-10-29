# VitaStation 1.6.4 beta

Esta versão consolida atualização automática, opções por jogo e integrações opcionais. Requer teste na consola física; a compilação e os testes de computador não substituem essa validação.

## Instalar e atualizar

Instalar esta versão uma vez pelo VitaShell, abrir o VitaStation para preparar o novo VitaStationBoot e reiniciar a consola. O atualizador das versões antigas não pode ser alterado retroativamente. A partir desta versão, uma atualização mais recente encontrada no GitHub é descarregada, verificada e preparada automaticamente. Um instalador temporário independente (VSTUP0001) fecha o launcher antes de promover o novo pacote, incluindo os recursos. Uma cópia verificada permite tentar recuperar a versão anterior em caso de falha. A limpeza do instalador temporário é tentada após o novo arranque.

Não interromper a instalação. Não há garantia de recuperação automática de um corte de energia. O resultado fica em `ux0:data/VitaStation/updates/install-result.txt`, as cópias em `ux0:data/VitaStation/pkg-staging/`. Se um corte deixar `updates/installing`, esse marcador pode bloquear o encaminhamento PS: terminar o instalador/reiniciar e remover apenas esse marcador pelo VitaShell antes de tentar novamente. Não remover os ficheiros de recuperação. Uma pasta residual `ux0:data/vst_update` bloqueia a instalação seguinte para preservar a recuperação.

## Triângulo e instalação de conteúdo

- **Atualizar jogo e instalar DLCs:** procura atualização oficial e DLCs da mesma ID no catálogo local; descarrega e instala sequencialmente pelo instalador nativo, atualizando a biblioteca ao concluir. Manter o VitaStation aberto durante esta operação. Esta ação não promete continuar ao abrir outro jogo; a fila nativa da loja continua a ser uma via separada.
- **Procurar cheats e mods:** reúne as pesquisas. Cheats obtidos ficam desativados. Mods exigem ZIP de Release verificável, estrutura rePatch reconhecida e ausência de conflitos. Outro X instala o resultado selecionado. Não instala qualquer ZIP arbitrário nem garante compatibilidade por jogo.
- **Preparar plugins:** obtém os componentes suportados, preservando escolhas e plugins existentes. Pode exigir reinício.
- **Desinstalar jogo:** confirmação começa em Cancelar. Protege aplicações do sistema e componentes do launcher. Para ISO/PBP, limita a remoção às localizações de jogos reconhecidas; a remoção nativa pode também remover dados associados.

A ação manual de imagem de fundo foi retirada; a procura automática permanece.

## Botões e aplicações da consola

PS sozinho mantém o encaminhamento para VitaStation; PS+L pede o LiveArea e suprime o retorno imediato. A abertura de aplicações usa um pedido confirmado ao plugin: fecha primeiro o VitaStation e o Shell lança o destino. Assim evita a confirmação de encerramento do VitaStation, sem alterar os diálogos das outras aplicações. Sem o novo plugin carregado, apresenta erro em vez de afirmar que abriu a aplicação. Reiniciar após instalar esta versão.

As definições têm uma área para aplicações originais detetadas em `vs0:app`: música, vídeos, fotografias, gestor de conteúdo, troféus e navegador, conforme os metadados disponíveis no firmware. As definições da consola sem implementação funcional deixam de aparecer; consultas suportadas permanecem identificadas como leitura. Não são simuladas alterações PSN, de troféus ou de rede indisponíveis.

## AutoPlugin II e Apollo

O catálogo AutoPlugin II é lido do projeto ONElua/AutoPlugin2 sem executar Lua nem abrir a aplicação. Recursos são obtidos de um commit fixado na consulta e verificados pelo hash Git e CRC quando fornecido. Instalação automática limitada aos adaptadores simples validados: shellbat, shellsecbat, pngshot, nosleep e ioplus. Os restantes componentes elegíveis são apenas descarregados para `ux0:data/VitaStation/autoplugin/downloads`, sem ativação automática. A compatibilidade de combinações de plugins exige teste na consola.

O VitaStation consulta os dados de patches de saves do Apollo pela ID, mantendo cache em `ux0:data/VitaStation/apollo`. Não altera saves automaticamente. Para gestão avançada, a ação Apollo pode instalar o VPK oficial verificado quando faltar, importar metadados ausentes e abrir o Apollo. O motor completo de saves não está incorporado.

Aplicações completas só são obtidas quando a função precisa delas. Auto Installer VPK não é necessário: é utilizado o instalador nativo já integrado. AutoPlugin II completo também não é dependência. Não se instala indiscriminadamente todo o catálogo. Backups e registo de propriedade da 1.6.3 continuam: plugins preexistentes/atualizados são preservados, sem downgrade; a limpeza adicional remove só novos ficheiros registados e repõe configurações conforme a escolha do utilizador.

## Validação

Passaram testes host/integração de menus, regiões, biblioteca, opções de jogo, cancelamento de remoção, parser AutoPlugin, atalhos e protocolo de abertura, integridade/cópia de todos os recursos da atualização e transações com falha/recuperação simuladas. O VPK é compilado e validado pelo VitaSDK. Instalação física, rede na Vita, atualização real, remoção, compatibilidade de plugins e comportamento do Shell ainda pendentes. Nenhuma destas verificações foi apresentada como teste físico concluído.

## Fontes das integrações

- https://github.com/ONElua/AutoPlugin2
- https://github.com/bucanero/apollo-vita
- https://github.com/bucanero/apollo-patches
- https://github.com/theheroGAC/Auto-Installer-vpk (avaliado; não incluído como dependência)
- https://github.com/coelhomarcus/usagi-pkgj
- https://github.com/r0ah/vitacheat
- https://github.com/dots-tb/rePatch-reDux0

As licenças e atribuições existentes continuam incluídas. O pacote de carátulas permanece na versão 1.04, separado do launcher.

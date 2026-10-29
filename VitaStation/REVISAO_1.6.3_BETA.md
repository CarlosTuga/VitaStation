# VitaStation 1.6.3 beta — plugins, fundos e conteúdo por jogo

## Backup a partir desta versão
No primeiro arranque desta versão, antes das alterações do VitaStation aos plugins, é criada a referência imutável em `ux0:data/VitaStation/safety-backup/baseline/`. O VitaShell não executa esta função durante a instalação do VPK: é necessário abrir o VitaStation. O primeiro arranque com cache força a preparação em segundo plano. As rotinas de alteração de plugins também exigem um backup verificado.

Inclui as pastas `ur0:tai`, `ux0:tai`, `ux0:pspemu/seplugins`, os módulos de `ux0:app/PSPEMUCFW/sce_module`, `ux0:vitacheat` e os ficheiros de plugins externos referenciados nos config.txt taiHEN reconhecidos. SHA-256 e tamanho são verificados; limites de 64 MiB por ficheiro e 512 MiB no conjunto. Uma cópia incompleta ou inválida bloqueia alterações que dependam dela. Não é uma imagem integral de toda a memória da consola.

Em consolas com VitaStation anterior, o estado atual ao primeiro arranque da 1.6.3 é a referência. Não é recriado um estado anterior à instalação antiga. Atualizações seguintes não substituem a referência.

## Duas escolhas de desinstalação
O ecrã começa em Cancelar, com a limpeza adicional desligada. Quadrado (o botão físico) alterna a caixa de seleção; Cima/Baixo percorre a lista de ficheiros afetados; Esquerda/Direita escolhe Cancelar ou Confirmar.

- Modo simples: remove VitaStation e carátulas e desativa/remove apenas VitaStationBoot; os restantes plugins ficam.
- Limpeza adicional: repõe os config.txt taiHEN e game.txt/vsh.txt/pops.txt PSP guardados e remove apenas plugins novos registados como acrescentados pelo VitaStation. O registo está em `owned-plugins.json` e o plano completo em `restore-plan.txt`, na pasta de segurança.

Plugins que já existiam são preservados por nome de ficheiro, incluindo cópias atualizadas. Módulos presentes do Adrenaline não são revertidos nem removidos. Não há comparação universal de números de versão dos módulos: a regra é conservar os binários existentes, evitando downgrades. Plugins novos modificados posteriormente por outro gestor são preservados se o SHA-256 já não coincidir com o registo. Plugins acrescentados manualmente depois não são marcados como propriedade do VitaStation.

Só um plugin original que esteja em falta pode ser recuperado do backup; um binário existente nunca é substituído por uma versão antiga nesta reposição. A reposição dos ficheiros de configuração reverte as alterações às configurações feitas depois do backup; a cópia `before-uninstall/` preserva o estado encontrado antes dessa operação. O backup e os dados de recuperação ficam guardados após remover o launcher. Reiniciar para descarregar o plugin da memória. Uma falha pode deixar uma remoção parcial e é apresentada como erro, sem afirmar sucesso.

## Fundos dos jogos
Além de pic0/pic1, pesquisa imagens PNG do LiveArea com outros nomes, selecionando uma imagem de dimensões adequadas. Quando não existe fundo, usa a imagem/ícone disponível do jogo para preencher o ecrã. Para IDs PS Vita nativas, tenta obter imagem do HexFlow e guarda em `ux0:data/VitaStation/backgrounds/ID.png`. A procura automática tem histórico limitado para evitar repetir pedidos em cada frame. A imagem de alternativa pode ser uma capa, não uma captura de ecrã. Não precisa de plugin de Internet. Falhas HTTPS não desativam a verificação dos certificados e mantêm a alternativa local.

## Triângulo — opções do jogo
O Triângulo na biblioteca abre seis ações:

1. Atualizações do jogo: consulta metadados oficiais Sony para IDs PCS…, compara APP_VER instalado e submete a atualização disponível ao LiveArea. Usa o cliente HTTPS nativo da Vita, com limites de leitura, conforme o mecanismo Usagi/PKGj.
2. Cheats: consulta o ficheiro PSV pela ID em r0ah/vitacheat, prepara VitaCheat Z06 para o alvo 3.65 quando necessário e guarda a base em `ux0:vitacheat/db/`. Códigos obtidos ficam desativados; a utilização exige selecionar os códigos no jogo e pode depender da versão do jogo.
3. Mods: pesquisa repositórios GitHub pela ID. Esquerda/Direita escolhe o resultado e outro X tenta instalar um ZIP da Release mais recente. Só aceita asset com SHA-256, limite de 256 MiB descarregado/512 MiB expandido e estrutura rePatch/ID ou ID. Recusa sobrepor ficheiros de mods existentes. Não há garantia de mods disponíveis ou compatíveis para cada jogo; repositórios sem Release/ZIP verificável não são instalados automaticamente.
4. Imagem de fundo: procura e guarda uma imagem de alternativa para a ID.
5. Preparar plugins: obtém VitaCheat/rePatch de fontes identificadas, verificando hashes fixos. Não substitui plugins já presentes da mesma família, incluindo forks detetados pelo nome. Plugins desativados pelo utilizador não são reativados à força. Reiniciar após instalação. Os binários VitaCheat não são redistribuídos neste VPK; são obtidos da fonte ao escolher a função.
6. Descarregar conteúdo do jogo: procura atualização e coloca todos os DLCs correspondentes à ID disponíveis no catálogo local da loja na fila nativa do LiveArea. Uma falha na consulta da atualização não impede procurar DLCs. Mostra aceites, falhas e indisponíveis. Os DLCs dependem dos TSV/JSON e metadados de instalação existentes; não cria conteúdo ausente do catálogo.

As operações automáticas de atualização/conteúdo e a base VitaCheat são para jogos PS Vita nativos, não atualizações de firmware nem um atualizador universal de PSP/PS1/homebrew. Cheats e mods dependem da compatibilidade do jogo. As descargas pelo LiveArea continuam pelo sistema; pesquisas, imagens, plugins e mods exigem manter o launcher aberto até concluir.

## Start
Retirado Início. O menu tem Definições, Som, Loja e Atualizar Biblioteca. O ícone de definições no topo continua a abrir as definições do launcher.

## Validação e limites
Passaram testes host/integração, cópia/imutabilidade/SHA-256 do backup, reposição apenas das configurações, preservação de módulos atualizados e plugins manuais, remoção de plugins novos registados, bloqueio por corrupção, menus Start/Triângulo e seleção da desinstalação, nomes alternativos de fundos, testes do instalador de carátulas e compilação/validação VPK.

Não foram executadas nesta versão a reposição/desinstalação nem as operações de rede/instalação numa consola física. Não é garantida recuperação automática de um corte de energia durante as trocas de ficheiros. A recuperação é conservada na pasta de segurança.

## Fontes
- Usagi/PKGj (BSD-2-Clause), metadados de atualização: https://github.com/coelhomarcus/usagi-pkgj/blob/master/src/patchinfo.cpp
- Base de cheats e pacote VitaCheat: https://github.com/r0ah/vitacheat
- rePatch 3.0: https://github.com/dots-tb/rePatch-reDux0
- Imagens: https://github.com/Andiweli/HexFlow-Covers
- URLs e hashes dos downloads opcionais: assets/game-support/sources.json

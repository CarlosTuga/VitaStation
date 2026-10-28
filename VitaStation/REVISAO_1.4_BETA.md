# VitaStation 1.4 beta

Compilado para PS Vita, APP_VER 01.40, TITLE_ID VTST00001. É uma versão de teste: a compilação e os testes no PC não comprovam funcionamento integral no hardware.

## Incluído

- Logótipo circular fornecido pelo utilizador, com exterior transparente, no ícone e no arranque.
- Loja interna por categorias: Vita, DLC, temas, demos, PSP, PSP DLC, PS1 e PSM. Lê TSV; usa JSON quando não encontra o TSV correspondente. Carrega os catálogos locais Usagi/PKGj.
- X num item da loja pede a descarga e a instalação automática. Triângulo abre a informação. Fila de oito pedidos, progresso e cancelamento por SELECT. Uma transação de instalação já iniciada deve terminar.
- Motor PKG adaptado do Usagi; transferência/verificação/extração de VPK para homebrews, preparação do head.bin e chamadas nativas ao instalador. PSP/PS1 são instalados como PBP para o ambiente PSP/Adrenaline. Não se afirma que esta versão crie todas as bubbles PSP no LiveArea.
- Carátulas PNG e JPEG locais, cache próprio e pedidos de PNG ao HexFlow quando faltam imagens. Não inclui um catálogo comercial nem garante uma carátula para cada título.
- Separação Jogos/Homebrews a partir dos catálogos locais Vita e dos metadados, com classificação manual persistente. Itens não identificados ficam em Homebrews. Não é uma autenticação criptográfica do editor.
- Leitura alternativa de metadados e imagens em ur0:appmeta e ux0:appmeta. Rejeita imagens vazias/inválidas e tenta outros recursos. Corrigida a substituição de slots do cache quando o número de imagens atinge o limite.
- Definições reorganizadas. Volume e pedido de brilho utilizam APIs nativas; volume tem leitura de confirmação. O SDK não fornece confirmação do brilho atual neste adaptador.
- Gestor individual dos plugins já configurados em taiHEN. Preserva secções, faz backup e protege plugins de arranque/armazenamento. Alterações aplicam-se após reiniciar; estar configurado não prova que o módulo foi carregado.
- Arranque automático opcional, desligado por defeito. O marcador habilita o plugin próprio VitaStationBoot; sem marcador deixa o LiveArea. SELECT permite saltar o arranque do launcher.
- Plugin próprio com encaminhamento PS: um toque pede abertura do VitaStation; dois pedem o LiveArea do VitaStation. Pressão longa do PS é preservada pelo reconhecimento do gesto. O comportamento físico ainda exige testes no firmware 3.65.
- Menu Energia com Desligar, Reiniciar e Cancelar, este selecionado por defeito. Disponível nas definições; o plugin tenta abri-lo ao manter Power premido cerca de 1,5 s. Não se afirma que substitua o botão físico em todos os estados, nem o desligamento forçado de emergência.

## Instalação

1. Instalar o VPK pelo VitaShell, como atualização do VitaStation existente.
2. Abrir o VitaStation. O primeiro arranque verifica e prepara as dependências na configuração taiHEN aplicável, com backup. A preparação automática está limitada ao firmware 3.65 confirmado pela API.
3. Reiniciar a consola quando a aplicação indicar. O arranque automático continua desligado até ser escolhido nas definições.

Plugins incluídos: NoNpDrm 1.2, NoPsmDrm 1.5, NoPspEmuDrm 1.06 (kernel/utilizador), npdrm_free_mod 7.3 como alternativa PSP, e o plugin próprio. Preserva o npdrm_free original se já existir. Os sources e licenças dos plugins distribuídos acompanham o pacote. Não instala indiscriminadamente o backup pessoal dos 54 plugins.

## Limitações de teste e funcionalidades ainda incompletas

- As novas instalações PKG/VPK, arranque, PS e Power precisam de teste real. Não houve execução remota de um instalador na consola; FTP serve para transferir e verificar ficheiros.
- A fila desta versão usa uma thread do launcher. Não implementa o serviço BGDL independente do sistema: pode parar se o sistema suspender ou terminar o VitaStation.
- As 95 funções do mapa não são 95 controlos implementados. Perfil PSN, troféus, rede, segurança e restantes serviços privados não estão integralmente controláveis nesta versão. Opções ainda indisponíveis são identificadas como tal e não abrem o painel original.
- PSM depende do runtime Sony existente. PSP/Adrenaline depende do ambiente PSP apropriado. Firmware ou runtimes proprietários não são redistribuídos neste VPK.
- Outros plugins de arranque são tratados como conflito, e não substituídos automaticamente. Os plugins essenciais ao armazenamento não podem ser desativados por este gestor.
- O linker do SDK emite uma nota sobre .note.GNU-stack no ficheiro crtn.o; os testes e a criação do pacote terminam com sucesso. Não é um teste de arranque no hardware.

## Verificações

Suite C de regressão com -Wall -Wextra -Werror; testes de transformações taiHEN, marcadores de arranque, gestos PS, controlo de erros, categorias, TSV/JSON, classificação e percurso de todas as definições; testes C++ do motor PKG com cabeçalhos malformados, caminhos de travessia, limites de descompressão e vetor SHA-256 conhecido. Validado ZIP, SFO e ícone PNG indexado.

O audit das imagens encontrou 105 imagens válidas e um ícone vazio em ur0:appmeta. Os ficheiros da consola usados nos testes ficam em work/, fora do VPK.

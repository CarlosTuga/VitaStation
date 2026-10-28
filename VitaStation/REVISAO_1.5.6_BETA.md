# VitaStation 1.5.6 beta

APP_VER 01.56, TITLE_ID VTST00001. Distribuído separadamente de Caratulas-de-loja.vpk 1.01, reduzido para cerca de 259 MB.

## Arranque em duas fases

A primeira fase carrega a interface, preferências e uma cópia local dos metadados da biblioteca. A segunda fase percorre as aplicações, atualiza a classificação e prepara os plugins numa thread própria. A interface não espera pelo fim dessa leitura. O logótipo deixa de ser reiniciado após a leitura da biblioteca.

O cache library-cache.bin tem versão, tamanho dos registos, limite de aplicações, validação de cadeias/enums e checksum. Um ficheiro inválido é ignorado. Os favoritos e estatísticas continuam no ficheiro de dados próprio. Alterações feitas na interface durante a leitura são preservadas na entrega da biblioteca. Sair antes da primeira leitura não grava uma biblioteca vazia por cima dos dados existentes.

O registo diagnostic.log identifica first_frame_ms, library_ready_ms e plugin_prepare_ms. Não foi medido um tempo de abertura nesta consola depois da alteração; os valores reais ficam para o teste no dispositivo.

## PKG no LiveArea

Pacotes PKG das categorias Vita, DLC, temas, PSP/PS1 e PSM passam pelo serviço SceDownload/ShellSvc do sistema, com a interface utilizada pelo Usagi PKGj. O cliente verifica o retorno do serviço e regista o ID da tarefa. Uma tarefa aceite fica entregue ao LiveArea e não depende da thread local do launcher. O progresso e cancelamento ficam nas notificações do sistema.

O pedido inclui a informação/licença fornecida pelo catálogo, o ícone persistente e o tipo do pacote. PSP/PS1 exige NoPspEmuDrm realmente carregado; PSM exige NoPsmDrm e o runtime PSM. A configuração dos plugins por si só não prova que estejam carregados. Os DLCs automáticos são submetidos depois do jogo, para a mesma Title ID, com resumo de pedidos aceites, falhas e indisponíveis. O sistema limita a fila a 32 tarefas.

A revisão evita apresentar «instalado» quando apenas submeteu uma tarefa. A instalação automática passa a ser gerida pelo serviço do sistema para esses PKGs, sujeita aos requisitos do pacote e à aceitação do LiveArea. Esta integração ainda precisa de teste real: o PC não executa ShellSvc nem confirma a instalação na Vita.

### VPK/ZIP de Homebrew

VPK, ZIP, dados e plugins continuam a usar o instalador local. Não foram convertidos artificialmente em PKGs nem apresentados como BGDL independente. Exigem manter o launcher aberto nesta revisão. O launcher impede uma abertura de jogo pela sua interface enquanto esse instalador local está ativo para evitar perder a operação. PKGs entregues ao LiveArea não mantêm esse bloqueio local.

O carregamento do instalador PAF/promoter foi adiado até à fase de instalação. Downloads VPK/ZIP por curl não inicializam desnecessariamente a antiga biblioteca HTTP/SSL da Vita.

## Carátulas e compressão

O launcher lê permanentemente ux0:data/VitaStation/covers/, incluindo JPEGs comprimidos e nomes alternativos estáveis para Homebrews. Também mantém a leitura das pastas locais Usagi/PKGj e o suporte opcional de assets empacotados. Não faz pedidos de carátulas à Internet nesta beta.

Caratulas-de-loja.vpk contém 20.205 ficheiros de imagem. As imagens opacas foram reduzidas para até 224x320 e JPEG qualidade 70; transparência conservada em PNG. O pacote passou de cerca de 777 MB para 259 MB (aproximadamente 66,7% menor). Os originais foram preservados no PC. Quatro ícones Alive continuam indisponíveis e nem todos os títulos dos TSVs têm uma imagem conhecida; COBERTURA.json enumera as faltas. Os nomes alternativos não representam jogos adicionais.

Instalar o VPK de carátulas, abrir «Carátulas de loja» e premir X para copiar a biblioteca permanente. Reabrir o VitaStation depois. Os ficheiros não expiram e não são apagados pelo reinício ou por uma atualização do launcher. Após a cópia, a aplicação instaladora pode ser removida sem apagar a biblioteca externa. O espaço físico necessário depende também do formato/tamanho dos clusters do cartão.

## Auditoria das definições

As 95 entradas foram percorridas. São 3 entradas de controlos nativos (volume/brilho e acesso ao volume), 15 consultas e 13 atalhos integrados; 64 permanecem sem adaptador. Estas contagens incluem entradas que abrem a mesma função e não equivalem a 31 novos serviços do sistema.

Foram ligados os atalhos de biblioteca, loja/instalação, plugins, arranque, PS, energia, menu rápido e preferências. As consultas incluem rede, relógio/fuso/hora de verão/formatos, idioma e botão de confirmação, espaço, bateria, estado de CPU/GPU, comando, registo, créditos e tarefas LiveArea. Consultar não significa editar: idioma, relógio e rede não passaram a ter escrita global.

Continuam pendentes, entre outras, edição/autenticação PSN, avatar, sincronização de troféus, emparelhamento Bluetooth, configuração/procura Wi-Fi e serviços de segurança/restauro. Cada entrada está identificada em mapping/AUDITORIA_1.5.6.csv, com estado e motivo. Não foram inventadas alterações ao registo nem APIs privadas para simular sucesso.

## Certificados

O registo da consola permitiu observar a falha na raiz 89 com erro OpenSSL 0606A041 (falha de alocação). O conjunto importado foi limitado a 39 raízes públicas identificadas por hash, para reduzir o consumo de recursos do OpenSSL nesta plataforma, mantendo verificação de cadeia e hostname. Inclui as raízes usadas pelos endpoints testados, incluindo Certainly, DigiCert e ISRG. Testes desktop com libcurl/OpenSSL validaram os endpoints GitHub/HexFlow e Archive.org; o resultado na Vita ainda requer confirmação. Não é apresentado como correção definitivamente comprovada na consola.

## Validação

Compilação ARM/VitaSDK e validação ZIP/SFO/ícone. Testes de regressão C, catálogos reais, limites/layout do pedido BGDL, recuperação de metadados de arranque, pesquisa de carátulas locais, roteamento de definições e controlos de erro. O teste TLS usa libcurl/OpenSSL reais no PC. O linker do SDK mantém a nota .note.GNU-stack em crtn.o.

O resultado visual e funcional das novas APIs nativas, principalmente BGDL, inicialização e consultas de sistema, deve ser confirmado depois de instalar o VPK. Não foi feita execução remota do launcher, de uma instalação de jogo ou de um reinício da consola.

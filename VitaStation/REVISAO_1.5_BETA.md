# VitaStation 1.5 beta

APP_VER 01.50, TITLE_ID VTST00001. Compilado com VitaSDK em D:\Vita sdk. Atualiza a aplicação existente.

## Loja → Homebrew: PS Vita Alive

- Subcategoria Homebrew com catálogo PS Vita Alive integrado, sem abrir a aplicação externa. Snapshot incluído: 1.150 itens, commit 16de7c63e9eadea46236bc11e7051e888ede7d7e.
- Consulta o catálogo incluído imediatamente; tenta atualizar em segundo plano a partir do endpoint público catalog.json. Quadrado pede nova atualização e tenta novamente as carátulas. Uma atualização inválida ou falhada preserva o catálogo anterior.
- Carátulas/ícones a partir das URLs do próprio catálogo, com cache por aplicação e URL. Downloads de imagens numa thread separada da instalação. Abrir a loja ou carregar carátulas não inicializa o instalador.
- X na grelha descarrega e instala a ligação principal recomendada. Triângulo abre informação, versão, autor, requisitos e ligações. Nas informações, setas escolhem VPK/dados/plugin; X instala a ligação escolhida. Triângulo permite ler a descrição e requisitos completos disponíveis; Círculo volta.
- VPK: HTTPS com redirecionamentos, validação TLS, suporte a tamanho desconhecido, extração ZIP, validação do SFO, geração de head.bin e chamada nativa ao promoter. Não permite substituir o launcher em execução nem títulos do sistema. Se o catálogo não fornecer uma ID válida, usa a ID real do SFO.
- Links MediaFire são resolvidos por leitura da página HTML, sem abrir navegador ou plataforma. Se o fornecedor mudar a página, apresenta um erro de resolução.
- ZIP com um único VPK pode ser instalado. ZIP de dados e ficheiros adicionais usam o destino do catálogo, limitado a ux0:data, ur0:data e à pasta da aplicação selecionada. Copia antes de aplicar; em caso de falha recupera os ficheiros anteriores. Mantém backups .VitaStation-*.bak dos ficheiros substituídos.
- Plugins selecionáveis do catálogo: CapUnlocker, ttbr_remap, azaharnative e ioPlus. Valida módulo SCE, secção KERNEL e configuração existente; preserva outra instalação ativa e guarda backups. Exigem reiniciar a consola. Outros plugins ainda sem adaptação apresentam erro explícito.
- Dados adicionais e plugins são ligações separadas; instalar o VPK principal não descarrega silenciosamente todas as outras ligações. Os requisitos do autor continuam aplicáveis. Ligações informativas não abrem uma aplicação externa nem são tratadas como instaladores.

## Outras correções incluídas

- Motor de carátulas HTTPS com certificados incluídos, mensagens de erro e cache invalidado quando chegam imagens novas.
- DLCs automáticos opcionais para jogos Vita, ligados por defeito e configuráveis nas definições: procura a mesma Title ID exata no catálogo local de DLCs, ignora duplicados, continua após falhas e apresenta um resumo. Não cruza regiões.
- Instalação do plugin de arranque já não exige que a versão pública reportada corresponda literalmente a 3.65; HENkaku pode alterar esse valor. O bridge é preparado antes das dependências opcionais, com configuração persistida e verificada em cada passo.
- Uma entrada desativada de plugin é apresentada como Desativado mesmo que o ficheiro não exista. O VDB antigo não é necessário ao VitaStation.
- Brilho: leitura da chave do sistema, alteração nativa e persistência com verificação; se a gravação falhar, tenta repor o valor anterior. Volume tem leitura de confirmação e HUD.
- Páginas de consulta do estado real da ligação, informação da consola e bateria. Estado de rede não é estado online da PSN.
- Opções sem implementação apresentam «Opção indisponível». O mapa de 95 funções é um inventário: não significa que todas sejam controlos implementados. Ver mapping/AUDITORIA_1.5.csv.
- O bridge próprio mantém a configuração opcional de arranque, PS simples/duplo e pedido de menu de energia. L+R+SELECT desativa o encaminhamento PS durante o arranque atual, como recuperação. Estas funções físicas continuam a precisar de teste na Vita.

## Instalação e teste na consola

1. Instalar VitaStation-1.5-beta.vpk pelo VitaShell como atualização.
2. Abrir o VitaStation e reiniciar quando indicar que os plugins foram configurados. Arranque automático continua opcional, desligado por defeito.
3. Abrir Loja → Homebrew. Verificar catálogo e imagens; Triângulo mostra os requisitos antes da instalação. X na grelha inicia logo o download.
4. Primeiro testar uma aplicação homebrew pequena que possa ser atualizada/instalada. Confirmar instalação concluída e abertura na biblioteca. O launcher não precisa de abrir PS Vita Alive ou Usagi para estes pedidos.

## Limites reais

Os testes no PC e a compilação não substituem testes de download, instalação, brilho e botões no hardware. FTP permite transferir/verificar ficheiros, não confirma execução do instalador.

A fila corre em segundo plano dentro do VitaStation: manter o launcher aberto durante a descarga e instalação. Não é o BGDL independente do sistema; suspender/encerrar o processo pode interromper o trabalho.

O adaptador não é uma cópia integral do cliente Alive. Não inclui a sua interface, downloads segmentados, retomada adaptativa, catálogo comercial adicional ou todos os instaladores especializados. ZIPs ambíguos, sem VPK ou sem destino válido são recusados com mensagem; extração limitada a 8 GiB e 65.536 entradas. A disponibilidade de cada URL/carátula depende dos servidores.

Avatar, edição da conta PSN, sincronização de troféus, emparelhamento Bluetooth, procura/configuração Wi-Fi e outras opções sem adaptador validado continuam indisponíveis. Não são apresentadas como correções concluídas.

Compilação terminada; permanece a nota do linker do SDK sobre .note.GNU-stack em crtn.o. Relatórios de testes acompanham o código-fonte. As pré-visualizações alive-grid/alive-details são capturas de layout no PC, com texturas substituídas por marcadores; não comprovam as carátulas na consola.

## Créditos

Curadoria e metadados originais: PS Vita Alive Store (PSVitaAlive) e contribuidores — https://github.com/VegettoSan/PSVitaAlive — CC BY 4.0 onde aplicável. Licenças em assets/alive. O catálogo foi integrado com outra apresentação e encaminhamento de ligações; os autores das aplicações mantêm os seus direitos. Não implica aprovação do VitaStation pelo projeto original.

Parser MediaFire adaptado do código MIT do PSVitaAlive; licença e origem em third_party/psvitaalive. As restantes licenças estão descritas em THIRD_PARTY.md. O arquivo de código-fonte não redistribui o backup pessoal dos plugins/configurações da consola.

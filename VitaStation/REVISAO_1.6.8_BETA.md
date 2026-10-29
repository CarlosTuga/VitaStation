# VitaStation 1.6.8 beta — contagem das carátulas e backup só dos plugins

Comparação com a versão anterior: [1.6.7 → 1.6.8](https://github.com/CarlosTuga/VitaStation/compare/v1.6.7-beta...v1.6.8-beta).

## Bugs e pontos alterados face à 1.6.7

- A deteção por uma única imagem podia aceitar um pacote incompleto. `cover_pack.c`, `update_covers.inc` e o novo `cover_inventory.h` passam a contar as entradas e comparar com o pacote real.
- `cover_install.inc` rejeita quantidade/tamanho incorretos e entradas repetidas, instalando diretamente os recursos sem abrir a aplicação externa.
- `self_update_stage.inc` elimina a cópia da aplicação e prepara o auxiliar do VPK novo, não o instalador antigo. `update_transaction.c` e `tools/updater/main.c` deixam de exigir ou tentar rollback na operação atual.
- `updater.h` e `ui.c` transportam/apresentam progresso em bytes/imagens; o auxiliar mantém a interface responsiva numa thread separada durante a promoção nativa.
- `audio.c`, `audio_queue.c`, `audio_synth.c` e `main.c` acrescentam o ficheiro de arranque PS5 identificado como startup.wav na coleção PS5 Sound Collection (Purrcival). É lido em blocos na thread de áudio e interrompido ao navegar, sem atrasar o arranque. Respeita a opção de efeitos e o volume da consola. Origem, licença declarada e atribuição em assets/audio/NOTICE.md e source.json; PCM original sem alteração das amostras. Não é um download oficial Sony nem uma afiliação. A autorização de redistribuição foi confirmada pelo responsável do projeto. Se o ficheiro estiver ausente, usa um som original sintetizado de alternativa.
- `launcher.c` repõe a abertura por nome com `EXIT_TO` (0x70000) e retira a alternativa por URI normal que deixava VitaStation ativo e provocava a confirmação de fecho. Aplica-se à biblioteca e às integrações, incluindo Apollo. Se a chamada falhar, só tenta a passagem confirmada ao Shell quando a preparação já terminou; não substitui o diálogo por uma mensagem a simular aceitação.
- `integrations.inc`, `system_ui.c` e `assets/autoplugin/` permitem abrir o catálogo local de 140 plugins sem esperar pela Internet. Triângulo atualiza a lista; X obtém/verifica o plugin. A cache guarda a referência do commit juntamente com os metadados. O ecrã Apollo passa a mostrar o estado/erro real da integração, antes ocultado pelo texto fixo. Os erros são registados em diagnostic.log. A gestão avançada de saves continua a usar Apollo, não é um motor de saves incorporado.
- `bootstrap.c`, `main.c` e `library_cache.c` separam reconstrução da biblioteca de preparação dos plugins e definem a pesquisa automática a cada dois arranques da consola. Reabrir o launcher na mesma sessão não incrementa o contador. A pesquisa manual e a atualização após instalar aplicações mantêm-se.

## Encerramento e validação na consola

Por FTP foi confirmada uma consola com 1.6.7 e um VitaStationBoot diferente do incluído nessa instalação. Foi substituído apenas o ficheiro próprio do bridge, com cópia anterior e verificação de bytes; config.txt e plugins externos não foram modificados. O bridge novo só é carregado depois de reiniciar. Não foram alteradas frequências, voltagens, perfis de energia ou partições de firmware. As cópias privadas não integram a publicação.

A abertura a partir do VitaStation pede a substituição automática do próprio launcher via EXIT_TO. O teste injeta falha/sucesso e confirma os flags e a ausência de fallback para a abertura comum com diálogo; a execução visual ainda requer teste físico. **Abrir outra bolha diretamente no SceShell/LiveArea enquanto o VitaStation já está suspenso não passa por esta função. Esta versão não interceta globalmente os diálogos do SceShell nem demonstra que esse caso externo está resolvido.**

## Desempenho e intervalo da biblioteca

Não instala o launcher numa partição do firmware. Usa a base local válida e só pesquisa quando necessário: cache em falta/inválida, segundo arranque, instalação detetada ou pedido manual. A preparação de componentes numa versão nova deixa de obrigar, por si só, a percorrer todas as aplicações. A reconstrução manual não repete a preparação de plugins. A identificação da sessão usa a hora de arranque estimada pela hora civil menos uptime do kernel, com tolerância de três segundos; ajustes grandes do relógio podem provocar uma pesquisa adicional. O contador antigo migra com uma reconstrução conservadora.

Não há overclock nem promessa de autonomia medida. Os ganhos procurados são menos I/O, menos preparação repetida e uso da cache; tempos e consumo desta versão ainda não foram medidos fisicamente.

## Quantidade instalada

O pacote Carátulas de loja 1.04 contém **21.811 imagens únicas**, confirmadas no manifesto do VPK distribuído. Os valores ficam em `include/cover_inventory.h`: versão 1.04, quantidade 21811 e tamanho do bloco 280712975 bytes. Ao mudar o pacote, estes valores têm de ser atualizados com os valores reais.

A instalação 1.04 guarda as imagens juntas em `ux0:data/VitaStation/covers/covers.bin`, com um índice `manifest.tsv`; não são 21.811 ficheiros separados. A contagem usa as entradas únicas desse índice e confirma que o tamanho total corresponde ao bloco existente. Índice inválido, nomes repetidos ou bloco truncado não são considerados uma instalação completa. Não relê os 280 MB para contar. O CRC de cada imagem continua verificado quando a loja a lê.

## Som de apresentação do jogo selecionado

Ao manter uma capa selecionada na biblioteca durante 650 ms, procura `sce_sys/snd0.at9` na atualização instalada (`ux0:patch/ID`), na aplicação (`ux0:app/ID`) e nas aplicações de sistema (`vs0:app/ID`). Usa o descodificador ATRAC9 nativo, numa thread separada, para áudio mono/estéreo de 44,1 ou 48 kHz, com volume reduzido e repetição. Não precisa de plugin adicional nem descarrega bandas sonoras.

Pára ao mudar de jogo, entrar num menu/loja, desligar os efeitos sonoros ou iniciar a aplicação. Aguarda o som de arranque para evitar sobreposição. Pedidos antigos de navegação são cancelados. Limita a leitura do ficheiro a 8 MiB e liberta o descodificador, buffers e porta de som ao terminar. `audio_preview.inc`, `audio.c`, `ui.c` e `main.c` implementam esta função.

Nem todos os jogos têm `snd0.at9` acessível ou compatível. Se não existir ou o descodificador o recusar, a seleção permanece silenciosa; o jogo continua a poder abrir. Não extrai música de dentro de ISO/PBP nem suporta ATRAC3 de PSP/PS1 nesta implementação. A descodificação real, repetição e mistura com os efeitos ainda precisam de validação na consola.

Para a estrutura antiga com imagens soltas, conta nomes de imagem reconhecidos, sem duplicar PNG/JPG com o mesmo nome base. Exige uma instalação completa num dos formatos; não soma parcialmente os dois formatos. Instalações com pelo menos a quantidade esperada são aceites, permitindo imagens adicionais. A contagem não demonstra que todas as imagens tenham o conteúdo correto.

## Descarga e instalação automáticas

Ao premir X em procurar atualizações, se faltarem carátulas, descarrega o VPK oficial, verifica tamanho e SHA-256, extrai diretamente o bloco/manifesto, verifica cada CRC e publica a nova instalação. Não abre nem exige executar a aplicação Carátulas de loja. Mostra a quantidade e barra de progresso. O pacote descarregado deve conter exatamente a quantidade/tamanho esperados; nomes repetidos são recusados. A fila prossegue automaticamente para a atualização do launcher quando existir uma nova versão.

A atualização do VitaStation continua a descarregar/verificar todos os recursos e a instalar automaticamente por auxiliar temporário. O auxiliar é obtido do novo VPK verificado, para não depender do comportamento de um instalador antigo. Não exige procurar o ficheiro ou confirmar X no auxiliar. A promoção precisa de outro processo porque o launcher é a aplicação substituída; o auxiliar mostra apenas o progresso e volta ao VitaStation. A barra representa bytes/imagens quando conhecidos e atividade sem percentagem na promoção nativa.

## Cópia de segurança

**Deixa de copiar a aplicação instalada e não exige manifesto de rollback da aplicação.** A atualização não cria cópia do launcher, carátulas ou biblioteca. Mantém o backup existente dos plugins, configurações taiHEN/PSP e componentes de plugins, protegido antes das funções que os alteram. Não modifica nem remove plugins anteriores nesta alteração.

Em caso de erro de instalação, mostra o erro e preserva os ficheiros preparados; não afirma que repôs a versão anterior da aplicação. Não remove previamente a aplicação instalada. Não há reposição automática do launcher nem garantia de recuperação de cortes de energia. Cópias de aplicação criadas por versões anteriores não são apagadas automaticamente por esta atualização.

## Instalar e testar

Os novos procedimentos entram em vigor depois de instalar esta versão. Uma atualização executada pela 1.6.4 ainda usa os procedimentos antigos até concluir. Se o plugin de abertura antigo ainda estiver carregado, instalar pelo VitaShell, abrir VitaStation e reiniciar para aplicar a correção de abertura incluída (plugin 1.6.6).

Testes: contagem parcial/completa de 21.811 entradas, duplicações, truncagem, manifesto inválido, CRC/instalação e preservação na falha/sem espaço, atualização sem backup da aplicação com sucesso/erro e sem tentativa de rollback, duração/limites/fade do som de alternativa, catálogo factual incluído, roteamento EXIT_TO, contador de dois arranques sem contar reaberturas, integridade do VPK, regressão host/integração e compilação VitaSDK aprovados. A instalação, audição do som, ATRAC9 e visualização das barras na consola física ainda precisam de teste. O caso externo do LiveArea descrito acima continua pendente.

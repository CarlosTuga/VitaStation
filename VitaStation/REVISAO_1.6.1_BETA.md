# VitaStation 1.6.1 beta — versão e atualizações

Nas Definições do VitaStation existem agora «Versão da aplicação» e «Atualizações do VitaStation».

X procura Releases públicas de CarlosTuga/VitaStation (incluindo betas), compara major/minor/patch e seleciona o VPK VitaStation mais recente. Outro X descarrega. Exige digest SHA-256 publicado pelo GitHub e tamanho até 64 MiB; só depois de conferir tamanho e hash publica o ficheiro final. Não guarda credenciais GitHub. A consulta é manual, não atrasa o arranque.

Destino: `ux0:data/VitaStation/updates/VitaStation.vpk`. Para concluir, fechar o launcher e instalar esse ficheiro no VitaShell. Não faz substituição automática do executável aberto. Não atualiza o pacote opcional de carátulas. Releases sem digest válido não são elegíveis. Rótulos beta/rc da mesma versão numérica não são ordenados entre si; incrementar patch em cada beta publicada. Consulta até 100 Releases.

TLS mantém a validação de certificados; a falha de HTTPS é apresentada no painel e não desativa a segurança. Na versão atual a descarga precisa do launcher aberto. Download limitado a 64 MiB em memória; a libertação é automática após guardar ou falhar.

Validação: testes host e integração passaram; teste específico cobre versão nova/antiga, repositório inválido, digest inválido e JSON inválido. VPK compilado com VitaSDK. A consulta e descarga na consola física ainda precisam de teste, particularmente devido ao histórico de problemas TLS.

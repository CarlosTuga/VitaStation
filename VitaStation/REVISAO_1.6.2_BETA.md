# VitaStation 1.6.2 beta — carátulas automáticas e desinstalação

## Atualizações e carátulas
Em Definições do VitaStation → Atualizações do VitaStation, X verifica as carátulas e procura versões. A verificação reconhece um bloco `covers.bin` com índice `manifest.tsv` válido e testa a leitura/CRC de uma imagem, ou imagens individuais PNG/JPG existentes. Uma pasta vazia não conta como instalação. Não é uma verificação integral de todas as imagens já instaladas.

Se não encontrar carátulas, procura `Caratulas-de-loja-1.04.vpk` nas Releases públicas do repositório CarlosTuga/VitaStation. Há alternativa pelo endereço da Release v1.6.0-beta se o pacote não estiver nas 100 Releases consultadas. Descarrega em fluxo para disco, exige tamanho e SHA-256 do GitHub, valida o índice e CRC de cada imagem, e instala automaticamente o bloco em `ux0:data/VitaStation/covers/`. Executa a instalação dos dados diretamente: não abre uma aplicação externa nem simula botões. O VPK temporário é removido depois. O índice e a cache de imagens são atualizados na sessão atual.

Se as carátulas estiverem presentes, não descarrega novamente o pacote. A consulta de versões do launcher mantém o funcionamento anterior: outro X descarrega uma versão mais recente; a instalação final do próprio launcher é feita no VitaShell. Não há descarga automática no arranque. A recuperação das carátulas é iniciada pela opção de atualização. Mantém a validação TLS; erros de certificado não são ignorados.

O pacote tem cerca de 257 MiB, instalado ocupa cerca de 268 MiB. Reservar pelo menos 600 MiB livres para descarregar e preparar o bloco. Manter o launcher aberto e a consola ligada até concluir. Falhas removem os temporários e a publicação usa cópias de segurança dos ficheiros anteriores; isto não garante recuperação de cortes de energia durante a troca final dos dois ficheiros.

## Desinstalar VitaStation e carátulas
A opção está nas Definições do VitaStation. A confirmação começa em Cancelar. Escolher Desinstalar tudo e confirmar com X grava as preferências de arranque/PS desligadas e remove os marcadores de arranque, PS e Power antes de trocar para `maintenance.self`, incluído no VPK. O executável de manutenção substitui o processo principal, sem criar outra bolha.

A manutenção volta a verificar a desativação, retira apenas as linhas exatas de VitaStationBoot.suprx das configurações taiHEN conhecidas, guarda cópias das configurações, remove a aplicação Carátulas de loja (`VSCV00001`), remove a pasta de carátulas e solicita ao gestor do sistema a remoção de VitaStation (`VTST00001`). Remove também os ficheiros temporários do pacote de carátulas e as cópias conhecidas de VitaStationBoot.suprx. Não elimina jogos, outros plugins, Adrenaline, preferências e backups de recuperação existentes fora da pasta de carátulas. Reiniciar depois de concluir para descarregar o plugin da memória e voltar ao arranque original.

Se uma operação falhar, apresenta o código de erro e não afirma que a remoção terminou. Pode haver remoção parcial das carátulas. Se o sistema recusar eliminar a aplicação em manutenção, concluir a remoção dessa bolha no LiveArea/VitaShell; a sequência automática de remoção ainda exige validação na consola física.

## Verificação
Testes host e integração; confirmação/cancelamento da desinstalação; deteção de bloco ausente, recarga de índice e CRC; seleção do pacote correto e rejeição de metadados inválidos; cópia do código de produção com ZIP/I/O simulados, CRC incorreto e falta de espaço; preservação de outros plugins e backup da configuração; commit/rollback do instalador de carátulas; compilação VitaSDK e validação do VPK.

Não foram executadas a descarga HTTPS nem a desinstalação numa consola física nesta versão. As demais limitações da versão beta continuam documentadas no projeto.

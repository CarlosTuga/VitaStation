# VitaStation 1.6.7 beta — menos leituras no atualizador

## Alterações

- Deteção do pacote de carátulas lê apenas a primeira entrada do manifesto e verifica o CRC dessa imagem. Não carrega, ordena nem invalida todo o índice da loja a cada pesquisa de atualização. É uma deteção de presença, não uma auditoria de todas as carátulas. A loja mantém a validação completa do índice ao carregá-lo e o CRC de cada imagem ao lê-la.
- Para instalações antigas com imagens individuais, percorre o diretório até encontrar a primeira imagem reconhecida, sem criar uma lista de todos os ficheiros.
- Retira a verificação integral do backup de plugins da etapa de atualização do VPK: esta operação não escreve plugins nem configurações taiHEN. As funções que efetivamente alteram plugins mantêm a proteção do backup. Não apaga nem enfraquece esse backup.
- Reutiliza a cópia de recuperação de uma tentativa anterior apenas quando a aplicação instalada e a cópia correspondem, ficheiro por ficheiro e SHA-256, ao mesmo manifesto. Uma versão igual, por si só, não basta. Se houver alteração, ausência ou corrupção, cria/verifica a cópia novamente.
- A consulta de identidade lê o cabeçalho do executável, em vez de carregar o executável inteiro repetidamente. A verificação integral de todos os ficheiros por SHA-256 antes da instalação permanece.
- Consulta as dez releases mais recentes em vez de cem. O pacote de carátulas continua a ter consulta própria à release de referência quando necessário.
- Regista `covers_probe_ms`, `rollback_ms` e se a cópia foi reutilizada em `ux0:data/VitaStation/diagnostic.log` para medir as etapas na consola.

Não elimina a cópia de recuperação nem promete atualização instantânea. A primeira cópia continua a exigir leitura/escrita e verificação dos recursos. A memória da consola, a rede e o tamanho da aplicação afetam a duração. Sem uma execução física desta versão, não há percentagem de aceleração medida.

## Instalação e limites

Atualizar pela opção do VitaStation ou instalar o VPK no VitaShell. Quem ainda tem a 1.6.4 usa o atualizador antigo até concluir esta instalação; o ganho só se aplica nas pesquisas/atualizações executadas pela nova versão. Abrir e reiniciar para garantir o plugin de abertura incluído, se ainda vier de 1.6.4/1.6.5. Mantém o plugin 1.6.6.

As correções de abertura da 1.6.5/1.6.6 permanecem. O diálogo de encerramento ao abrir outra bolha diretamente no LiveArea com VitaStation suspenso continua por resolver. Ver [limites 1.6.6](REVISAO_1.6.6_BETA.md).

## Verificação

Testes de presença das carátulas sem percorrer o manifesto inteiro, imagem ausente/corrompida, leitura/CRC da loja, igualdade/corrupção da cópia, integridade completa e transações simuladas de atualização aprovados. Regressão host e compilação/validação VitaSDK aprovadas. Medição dos tempos e instalação física desta versão ainda pendentes.

# Carátulas de loja 1.02 — instalação sequencial

O VitaStation 1.5.6 não foi alterado. Esta revisão atualiza apenas o instalador das imagens, mantendo TITLE_ID VSCV00001 e o diretório permanente ux0:data/VitaStation/covers/.

## Operações reduzidas

- VPK passa de mais de 20 mil entradas de imagem para um único assets/covers.bin, mais o manifesto e licenças. As imagens e a qualidade são exatamente as da versão comprimida 1.01.
- A cópia abre esse bloco uma vez e lê segmentos sequenciais, em vez de abrir e fechar um ficheiro de origem por carátula.
- Os dados lidos são validados pelo CRC do manifesto; escritas e fecho são verificados. As sincronizações passam a ser agrupadas a cada 128 imagens e no final.
- Deixa de reler todos os ficheiros acabados de gravar. A verificação completa do destino mantém-se para ficheiros existentes ao retomar; a versão anterior fazia uma segunda leitura e sincronização forçada em cada cópia nova.
- Atualização visual limitada a cerca de seis vezes por segundo, em vez de esperar um fotograma a cada quatro imagens. Espaço livre consultado por lotes, mantendo o tratamento de erros de escrita.
- Mantém temporários, preservação de originais, recuperação de renomeações e possibilidade de continuar após parar. Depois de uma interrupção, reabrir o instalador verifica os ficheiros existentes antes de os ignorar. O log inclui duração da cópia.

A pasta de destino ainda contém as 20.205 imagens individuais, como o launcher espera. A criação desses ficheiros e o desempenho do cartão continuam a limitar a velocidade. Não há medição desta versão na Vita; não é prometido um tempo exato.

Se as carátulas já estiverem completamente instaladas, não é preciso voltar a copiá-las. Se a cópia ainda estiver a decorrer, pode terminar normalmente ou parar com Círculo e usar esta atualização para continuar. Não iniciar uma atualização pelo VitaShell enquanto o instalador antigo está a copiar.

Na primeira atualização, o VitaShell pode ainda demorar a remover os milhares de ficheiros internos da versão antiga. As atualizações posteriores já usam o bloco único. As imagens externas já copiadas não são removidas pela atualização.

Compilação Vita e testes de CRC, leitura sequencial, saltos de segmentos ao retomar, leituras incompletas, preservação e recuperação passaram no PC. A validação do VPK confirma CRC de todo o bloco e de cada uma das 20.205 imagens. A rapidez e funcionamento reais ainda requerem confirmação na consola.

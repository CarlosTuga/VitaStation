VitaStation 1.5.7 beta

Arranque: carrega a base validada ux0:data/VitaStation/library-cache.bin. Nos arranques normais não inicia a pesquisa de aplicações nem a preparação de plugins em segundo plano. Carrega os ícones visíveis a partir dos caminhos guardados; não descodifica todos os ícones antecipadamente. Logótipo reduzido para aproximadamente 0,4 s quando existe base válida.

Reconstrução: primeiro arranque, base ausente/corrompida, a cada 10 aberturas do VitaStation desde a última reconstrução bem sucedida, ou Definições > VitaStation > Reconstruir base de dados. Conta aberturas da aplicação, não reinícios físicos do firmware. Depois de instalações locais, a biblioteca também é atualizada. Durante uma reconstrução a pesquisa pode decorrer em segundo plano; nos restantes arranques não ocorre. Se a reconstrução falhar, volta a tentar na abertura seguinte. A base anterior permanece utilizável.

Carátulas: compatível com os ficheiros individuais anteriores e com o novo pacote em bloco do instalador 1.03. Caminho permanente ux0:data/VitaStation/covers/covers.bin e manifest.tsv. O índice é carregado ao abrir as carátulas da loja; procura binária por nome e leitura apenas da imagem pedida, com CRC e limites de dimensões. Não necessita de acesso à Internet.

Validação: testes host e integração, contador de 10 aberturas, rejeição de bases corrompidas, índice/offsets/CRC de imagens, compilação VitaSDK e validação do VPK aprovados. Velocidade real e funcionamento nesta consola ainda por medir. Mantêm-se as limitações anteriores das definições sem adaptador e das descargas VPK/ZIP locais.

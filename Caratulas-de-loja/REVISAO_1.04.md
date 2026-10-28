Carátulas de loja 1.04

Diagnóstico: o pacote anterior continha 0 dos 676 IDs PSM e apenas 114 dos 1173 IDs PS1 do catálogo (excluindo ASIA). No PS1, as imagens HexFlow estavam maioritariamente nomeadas pelo serial físico SLUS/SCUS/SLES/SCES, enquanto o TSV usa frequentemente os IDs PSN NPUJ/NPUI/NPEE/NPEF/NPJJ. O caminho de leitura do VitaStation já estava correto.

Correção: ícones PSM obtidos diretamente de PSM Reborn, validados e redimensionados para o limite do pacote. Aliases PS1 criados para os IDs PSN com nomes exatos normalizados e correspondências de edições/nomes revistas, conservando os pixels das imagens existentes. Sem correspondências aproximadas automáticas. Fontes de nomes/seriais: libretro-database e DuckStation; relatórios de origem incluídos.

Para as regiões Europa e EUA agora exibidas pelo VitaStation: 374 de 379 IDs PS1 cobertos; 413 de 413 IDs PSM cobertos. Os 5 IDs PS1 restantes são coletâneas/bundles sem imagem confirmada no acervo; continuam com o visual de ausência de imagem, sem lhes atribuir uma carátula de outro produto. Relatório COBERTURA_PS1_PSM_1.04.json. Fora destas regiões também foram incluídos os ícones PSM disponíveis e os aliases PS1 confirmados; não implica cobertura total de todo o catálogo.

Mantém o formato em bloco da versão 1.03, na pasta ux0:data/VitaStation/covers/, com covers.bin e manifest.tsv. Requer VitaStation 1.5.7 ou posterior. Instalar VPK, abrir Carátulas de loja no LiveArea, premir X e aguardar conclusão; depois fechar/reabrir VitaStation. Não elimina imagens individuais antigas. Necessita de espaço para o bloco novo e conserva o bloco anterior como .previous. A cópia em bloco anterior terminou na consola com 20205/20205 imagens e 0 falhas em 540530 ms (~9 min); ainda não foi medida a cópia desta versão maior.

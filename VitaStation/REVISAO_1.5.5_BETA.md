# VitaStation 1.5.5 beta — dois VPKs

APP_VER 01.55, TITLE_ID VTST00001. Pacote pequeno do launcher, separado de Caratulas-de-loja.vpk conforme a decisão final do utilizador.

A loja lê as imagens permanentes em ux0:data/VitaStation/covers/ sem pedidos à Internet. Mantém a consulta das pastas locais Usagi/PKGj. Acrescenta suporte a imagens incluídas em app0:assets/store-covers caso uma compilação futura as inclua; este VPK pequeno não contém a coleção de imagens. O instalador separado copia-as para a pasta externa permanente.

Mantém chaves Homebrew exatas e nomes alternativos estáveis, para não perder a imagem quando muda a URL do catálogo. Reiniciar não elimina os ficheiros. Catálogos TSV/JSON locais e catálogo Alive incluído continuam disponíveis. Demos e regiões ASIA/Asian permanecem desativadas na loja.

Instalar o launcher, instalar o VPK «Carátulas de loja» e abrir essa aplicação uma vez para premir X. Depois, reabrir o VitaStation. Só instalar a bubble de carátulas no VitaShell não executa a cópia para a biblioteca externa.

Regressão, catálogos reais e pesquisa de imagens permanentes/empacotadas passaram no PC; compilação Vita e validação VPK concluídas. O resultado no hardware ainda requer teste. O erro HTTPS das transferências de aplicações não é declarado corrigido por esta atualização: as carátulas locais contornam a necessidade de HTTPS para imagens. Restantes limites das versões anteriores mantidos.

# Componentes de terceiros

- Usagi PKGj: coelhomarcus/usagi-pkgj, commit 744541320532b59335771de67399fdeb231f7284; BSD-2-Clause. Cópia adaptada em third_party/usagi. puff.c/.h mantêm a licença zlib.
- cJSON 1.7.19: MIT, third_party/cJSON.
- fmt 10.2.1: licença incluída em third_party/fmt.
- VitaShell: template head.bin e algoritmo de preparação adaptado, Copyright TheFloW; GPL-3.0-or-later. Origem fixa e licença em third_party/vitashell. O código adaptado consta de src/store_vita.cpp; a distribuição do derivado exige cumprir a GPL, incluindo disponibilizar o código-fonte correspondente. O arquivo de código-fonte acompanha este compilado.
- Plugins NoNpDrm, NoPsmDrm, NoPspEmuDrm e npdrm_free_mod: versões, commits, URLs e hashes nos manifestos assets/plugins; fontes completas e licenças em third_party/plugin-sources. Também incluídos no VPK.
- VitaStationBoot: código original do projeto, disponibilizado sob MIT; não incorpora o código do AutoBoot. Usa APIs declaradas no VitaSDK. A técnica de observar o arranque do LiveArea foi consultada na implementação pública AutoBoot.
- Bibliotecas do VitaSDK: seguem as respetivas licenças. A compilação usa as versões instaladas no SDK local; o SDK não acompanha o VPK.

O logótipo PlayStation fornecido pelo utilizador não é uma licença sobre marcas Sony. O aspeto visual do launcher não implica associação oficial.

console-backup contém uma cópia pessoal de plugins da consola, não uma seleção de dependências redistribuíveis. Verificar as licenças de cada um antes de publicar esse backup.


## Integrações 1.5

- PS Vita Alive Store / VegettoSan/PSVitaAlive, commit 16de7c63e9eadea46236bc11e7051e888ede7d7e: catalog.json, licença da curadoria CC BY 4.0 onde aplicável, exceções legadas conforme CATALOG_LICENSE.md. Catálogo e créditos incluídos em assets/alive. Parser MediaFire adaptado de Client PSVitaAlive/source/network/mediafire_resolver.cpp, licença MIT em third_party/psvitaalive/LICENSE. A apresentação e o encaminhamento de instalações foram alterados no VitaStation. Abordagem de I/O ZIP 64-bit e promoter superficial consultada no mesmo projeto; não se afirma paridade com todos os seus instaladores.
- libcurl 8.17, ligada através do VitaSDK: https://curl.se/docs/copyright.html. HTTPS valida a cadeia e o hostname; não existe fallback que desligue a verificação.
- Mozilla CA bundle distribuído por curl: https://curl.se/docs/caextract.html ; certificados em assets/cacert.pem, extração 2026-10-20. Metadados de origem no próprio ficheiro; termos Mozilla https://www.mozilla.org/MPL/2.0/.


Integração BGDL 1.5.6: third_party/usagi/bgdl.cpp/.hpp adaptados de coelhomarcus/usagi-pkgj, commit 744541320532b59335771de67399fdeb231f7284, BSD-2-Clause; preservados créditos de RE dots_tb, CelesteBlue123, SilicaDevs, possvkey e NPS Team. Alterações: validação dos metadados, estruturas/layout verificados, alocação correta do cabeçalho, verificação de exports, RIFs no diretório próprio e retorno do ID da tarefa.
Dependência de SDK taihen.tar.xz: https://github.com/vitasdk/packages/releases/download/master/taihen.tar.xz ; SHA-256 afced213d9d308a665ef91809fd728614a92ec31b2fcf61cf1f6a3e98bec4014. É dependência de compilação, não instala outro plugin na consola.


## PSP/PS1 runtime added in 1.6.0
Runtime payload and per-bubble loader from RetroFlow v8.5.0. See assets/psp-runtime/CREDITS.txt, included licenses and ORIGINS.json. Public AdrBubbleBooter-oss source snapshot is bundled under third_party/plugin-sources/AdrBubbleBooter-oss-source.zip and inside the VPK. Original Sony firmware is not included.

## Som de arranque PS5 (1.6.8)

Recurso separado da licença do código. Origem: PS5 Sound Collection, Purrcival, https://archive.org/details/ps5_sound_collection. O arquivo declara CC BY-ND 4.0. Direitos dos respetivos titulares; autorização de redistribuição confirmada pelo responsável do projeto. Ver assets/audio/NOTICE.md e source.json para atribuição, formato e hash.


## Fontes incluídas na 1.6.14

Rajdhani Medium e Audiowide Regular, distribuídas sob SIL Open Font License 1.1. Ficheiros e licenças completas em `assets/fonts/`. Fontes originais: https://github.com/google/fonts/tree/main/ofl/rajdhani e https://github.com/google/fonts/tree/main/ofl/audiowide . As fontes não foram modificadas. Os temas visuais do VitaStation são desenhos procedurais originais inspirados em estilos de consolas, sem relação oficial com a Sony.

# Compilar e verificar

Estrutura: o launcher está em VitaStation/ e o instalador em Caratulas-de-loja/. Ambos conservam os seus CMakeLists e as fontes. Não é incluído um SDK.

## Launcher em Windows

Na raiz da preparação:

    python -m pip install cmake ninja
    powershell -NoProfile -ExecutionPolicy Bypass -File .\VitaStation\build-vita.ps1 -SdkPath 'D:\Vita sdk'

O script configura VITASDK/PATH durante a execução e restaura os valores anteriores. Saída: VitaStation/build-vita/VitaStation.vpk. Pode ser escolhido outro diretório com -BuildDirectory.

Dependências de compilação: VitaSDK e bibliotecas/stubs usados em VitaStation/CMakeLists.txt, incluindo vita2d, FreeType, PNG, JPEG, ZIP, bzip2, curl, OpenSSL, lzma, zstd, zlib, taiHEN e os stubs de sistema correspondentes, incluindo SceSqlite. A preparação original utilizou GCC 15.2.0. Não se promete reprodução binária com qualquer SDK ou toolchain.

## Outro ambiente com VitaSDK configurado

Definir VITASDK e acrescentar o bin do SDK ao PATH, depois:

    cmake -S VitaStation -B VitaStation/build-vita -DVITA=ON
    cmake --build VitaStation/build-vita --parallel 2
    python VitaStation/tests/validate_vpk.py VitaStation/build-vita/VitaStation.vpk

O script PowerShell verifica executáveis Windows; o caminho CMake direto é o ponto de entrada para outras plataformas com as dependências adequadas.

## Instalador de carátulas

Com VITASDK e o compilador disponíveis:

    cmake -S Caratulas-de-loja -B Caratulas-de-loja/build-vita
    cmake --build Caratulas-de-loja/build-vita --parallel 2

Isto compila o SELF e metadados. Para reconstruir o VPK completo são necessários os pixels das imagens. Eles não estão duplicados na árvore Git: estão em assets/covers.bin dentro do VPK de carátulas da Release.

As ferramentas Caratulas-de-loja/tools/export_images.py e package_vpk.py permitem exportar e voltar a empacotar. Consultar as mensagens de utilização no início desses scripts. O projeto usa o ícone do launcher na pasta irmã VitaStation/assets/icon0.png; conservar a estrutura conjunta.

O manifesto, CRCs, inventário SHA-256, relatórios e créditos estão incluídos. Não substituir o pacote por imagens arbitrárias sem atualizar os manifestos e respeitar as licenças dos recursos.

## Testes host

    python -m pip install ziglang
    python VitaStation/tests/run_host.py --zig
    python VitaStation/tests/run_integration.py
    python VitaStation/tests/run_media.py
    python VitaStation/tests/run_launch_handoff.py
    python VitaStation/tests/run_cover_pack.py
    python Caratulas-de-loja/tests/run_tests.py

Teste do preparador de bolhas, com a fixture de upstream explícita:

    python VitaStation/tests/run_bubbles.py CAMINHO/RetroFlow_v8.5.0.vpk

Fixture: https://github.com/jimbob4000/RetroFlow-Launcher/releases/tag/v8.5.0

O teste usa os módulos originais desse pacote e chamadas de consola simuladas. Não instala plugins nem escreve numa Vita.

## Validação da publicação

    python tools/verify_publication.py

Verifica conteúdos obrigatórios, integridade dos pacotes, ficheiros grandes fora do Git e ausência de referências privadas conhecidas nos textos de publicação. Não constitui uma auditoria de segurança completa.

# Compilação no SDK instalado

A instalação real em `D:\Vita sdk` foi verificada e utilizada para compilar ARM, ligar, criar o SELF e gerar o VPK nesta sessão.

Projeto preparado em `D:\Vita sdk\VitaStation`. Para voltar a compilar, abre `Compilar-VitaStation.cmd`, ou executa:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File "D:\Vita sdk\VitaStation\build-vita.ps1"
```

Saída: `D:\Vita sdk\VitaStation\build-vita\VitaStation.vpk`.

O script suporta o espaço no caminho, configura VITASDK/PATH apenas durante o processo e restaura os valores anteriores. Não modifica permanentemente as variáveis de ambiente.

Preparação efetuada: CMake e Ninja instalados através dos respetivos pacotes Python; bases de pacotes do vdpm sincronizadas; instalados `libvita2d`, `freetype`, `libjpeg-turbo`, `libpng`, `zlib` e `bzip2`. O projeto procura `png16` ou `png` e liga também `bz2`, necessário para esta versão do FreeType. O gestor de pacotes emitiu avisos sobre aliases/symlinks libpng no Windows; o build usa diretamente o arquivo real `libpng16.a` e terminou com sucesso.

Compilador: arm-vita-eabi GCC 15.2.0. SDK identificado por `version_info.txt`: snapshot construído em 25/08/2026. A compilação e o packaging reais foram verificados. A execução, instalação, áudio, desempenho e lançamento de jogos numa PS Vita continuam por testar. O VPK não foi instalado automaticamente em nenhuma consola.

Foi também corrigido no CMake do projeto o tratamento dos argumentos de recursos com espaços, usando comandos de packaging VERBATIM. Não foi necessário editar o vita.cmake instalado. O script foi executado tanto em PowerShell atual como em Windows PowerShell 5.1.

## Correcao de instalacao — 28/10/2026

Apos o erro 0x8010113D comunicado na consola, verificou-se que icon0.png era RGBA (tipo PNG 6). Foi convertido para PNG de paleta, 8 bits (tipo 3), 128x128 e sem entrelacamento. O VPK foi novamente gerado pelo SDK. O script de build passou a verificar os recursos PNG e os CRCs. O validador rejeitou o pacote RGBA anterior e aceitou o novo pacote. Falta confirmar a instalacao na consola. Referencia: https://github.com/TheOfficialFloW/VitaShell/issues/312

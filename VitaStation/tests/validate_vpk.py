"""Check package integrity and indexed, non-interlaced installation artwork."""
import struct
import sys
import zipfile
import zlib

def validate(path):
    with zipfile.ZipFile(path) as package:
        if package.testzip() is not None:
            raise ValueError('Invalid ZIP CRC')
        required = {'eboot.bin', 'sce_sys/param.sfo', 'sce_sys/icon0.png'}
        if not required.issubset(package.namelist()):
            raise ValueError('Missing required VPK entries')
        for name in package.namelist():
            if not name.startswith('sce_sys/') or not name.endswith('.png'):
                continue
            data = package.read(name)
            if data[:8] != b'\x89PNG\r\n\x1a\n':
                raise ValueError('Invalid PNG: ' + name)
            w, h, depth, color, compression, filtering, interlace = struct.unpack_from('>IIBBBBB', data, 16)
            if (depth, color, compression, filtering, interlace) != (8, 3, 0, 0, 0):
                raise ValueError('Artwork must be indexed 8-bit, non-interlaced: ' + name)
            if name == 'sce_sys/icon0.png' and ((w, h) != (128, 128) or len(data) > 128 * 1024):
                raise ValueError('Invalid icon dimensions or size')
            offset = 8
            chunks = []
            while offset < len(data):
                size = struct.unpack_from('>I', data, offset)[0]
                end = offset + 12 + size
                if end > len(data):
                    raise ValueError('Truncated PNG: ' + name)
                body = data[offset + 4:offset + 8 + size]
                crc = struct.unpack_from('>I', data, offset + 8 + size)[0]
                if zlib.crc32(body) != crc:
                    raise ValueError('Invalid PNG CRC: ' + name)
                chunks.append(data[offset + 4:offset + 8])
                offset = end
            if not chunks or chunks[0] != b'IHDR' or chunks[-1] != b'IEND' or b'PLTE' not in chunks or b'IDAT' not in chunks:
                raise ValueError('Missing PNG chunks: ' + name)
            print(f'{name}: {w}x{h}, indexed 8-bit, non-interlaced, CRC OK')
    print('VPK package checks passed; installation on hardware still requires testing.')

if __name__ == '__main__':
    validate(sys.argv[1])

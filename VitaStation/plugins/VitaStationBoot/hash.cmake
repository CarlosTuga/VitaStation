file(SHA256 "${INPUT}" digest)
file(WRITE "${OUTPUT}" "#define VS_BOOT_SHA256 \"${digest}\"\n")

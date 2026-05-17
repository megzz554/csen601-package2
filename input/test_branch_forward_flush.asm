# Taken JEQ with a forward offset; skipped instructions must be flushed.
MOVI R1 1
MOVI R2 1
JEQ R1 R2 2
MOVI R3 999
MOVI R4 7
SUB R5 R4 R1

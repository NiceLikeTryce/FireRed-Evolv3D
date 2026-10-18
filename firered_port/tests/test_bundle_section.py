"""Prevent BSS/native audio symbols becoming resident payload offsets."""
import sys,tempfile,subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[2];sys.path.insert(0,str(root/"3ds_port/scripts"))
from ctr_bundle import defined_symbols,tool
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/"fixture.s").write_text('.section native_payload,"a",%progbits\n.global payload_symbol\npayload_symbol: .word 7\n.section .bss\n.global native_audio_state\nnative_audio_state: .space 16\n.global absolute_symbol\n.set absolute_symbol, 12\n')
 subprocess.run([tool("arm-none-eabi-as"),"-o",str(p/"fixture.o"),str(p/"fixture.s")],check=True)
 assert defined_symbols(p/"fixture.o","native_payload")=={"payload_symbol":0}
 try:defined_symbols(p/"fixture.o","missing")
 except ValueError:pass
 else:raise AssertionError("Missing payload section accepted")
print("PASS resident symbol isolation: BSS/absolute symbols excluded; missing section rejected")

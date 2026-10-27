"""Check the full-size mouse-create regression console log."""
import sys
from pathlib import Path
text = Path(sys.argv[1]).read_text(encoding="utf-8", errors="replace")
text = text[text.rfind("RB_FULLBENCH_READY"):]
assert text.count("FTEBROOM_TRANSFORM_PASS create78_brush_count_") == 4, text[-5000:]
assert "brushes=30328 selected=1" in text and "brushes=30327 selected=0" in text
assert not any(item in text for item in ("CSQC_Abort", "runaway loop error", "TRANSFORM_FAIL")), text[-5000:]
print("PASS: full-size mouse create, preview, release, Undo, Redo and Delete")

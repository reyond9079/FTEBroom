"""Validate the full-size retained-edit regression and canonical document snapshots."""
import argparse
import shlex
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument('--log', type=Path, required=True)
parser.add_argument('--fast', type=Path, required=True)
parser.add_argument('--rebuilt', type=Path, required=True)
parser.add_argument('--live-image', type=Path)
parser.add_argument('--rebuilt-image', type=Path)
parser.add_argument('--viewport', type=int, nargs=4, help='Exact image comparison rectangle: left top right bottom')
args = parser.parse_args()
log = args.log.read_text(errors='replace')
log = log[log.rfind('RB_FULLBENCH_READY'):]
assert not any(word in log for word in ['TRANSFORM_FAIL', 'CSQC_Abort', 'runaway loop error']), log[-6000:]
for mode in [1, 2, 3]:
    assert 'RB_INCREMENTAL81 mode=' + str(mode) + ' ' in log
assert log.count('TRANSFORM_PASS incremental81_gesture_committed') == 3
assert log.count('TRANSFORM_PASS incremental81_keeps_base_geometry') == 3
assert log.count('TRANSFORM_PASS incremental81_repeated_create_undo_redo') == 16
assert 'TRANSFORM_PASS incremental81_batch_delete_keeps_base' in log
assert log.count('TRANSFORM_PASS incremental81_material_counts_match_full_scan') >= 3
assert log.count('TRANSFORM_PASS incremental81_overlay_candidates_match_full_scan') >= 3

def canonical(path):
    nodes = []
    for line in path.read_text().splitlines():
        words = shlex.split(line)
        if len(words) == 7 and words[0].lstrip('-').isdigit():
            node = dict(key=tuple(map(int, words[:3])), parent=int(words[3]),
                        layer=int(words[4]), flags=int(words[5]), name=words[6], fields={})
            nodes.append(node)
        elif len(words) == 2:
            node['fields'][words[0]] = words[1]
    result = {}
    for node in nodes:
        if node['key'][0] < 0:
            continue
        parent, layer = node['parent'], node['layer']
        node['parent'] = nodes[parent]['key'] if parent >= 0 else None
        node['layer'] = nodes[layer]['key']
        assert node['key'] not in result
        result[node['key']] = node
    return result

assert canonical(args.fast) == canonical(args.rebuilt), 'Retained and full document trees differ'
if args.live_image or args.rebuilt_image:
    from PIL import Image, ImageChops
    assert args.live_image and args.rebuilt_image
    live, rebuilt = Image.open(args.live_image).convert('RGB'), Image.open(args.rebuilt_image).convert('RGB')
    assert live.size == rebuilt.size
    if args.viewport:
        x0, y0, x1, y1 = args.viewport
        assert 0 <= x0 < x1 <= live.width and 0 <= y0 < y1 <= live.height
        live, rebuilt = live.crop(args.viewport), rebuilt.crop(args.viewport)
    assert ImageChops.difference(live, rebuilt).getbbox() is None, 'Retained and full images differ'
print('PASS: real create/move/vertex commits, 16 Undo/Redo cycles, batch delete, material counts, canonical document and optional exact images')

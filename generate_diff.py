
import difflib
import sys
import os

file1_path = "Spec_FlowZone_Looper1.3 backup.md"
file2_path = "Spec_FlowZone_Looper1.3.md"
output_path = "diff_report.html"

# If arguments are provided override the default paths
if len(sys.argv) > 3:
    file1_path = sys.argv[1]
    file2_path = sys.argv[2]
    output_path = sys.argv[3]

with open(file1_path, 'r') as f1, open(file2_path, 'r') as f2:
    lines1 = f1.readlines()
    lines2 = f2.readlines()

# wrapcolumn=None lets the browser handle wrapping via CSS
differ = difflib.HtmlDiff(wrapcolumn=None) 
html_diff = differ.make_file(lines1, lines2, file1_path, file2_path)

# Custom CSS to repair the layout
style_tag = """
<style type="text/css">
    body { margin: 0; padding: 10px; background: #fff; }
    
    table.diff {
        font-family: Menlo, Monaco, Consolas, "Courier New", monospace;
        border: 1px solid #ccc;
        border-collapse: collapse;
        width: 100%;
        table-layout: fixed; /* Important: this enforces the width splitting */
    }
    
    /* Line Number Columns (1st and 3rd cols) */
    td.diff_header {
        width: 3em; /* Fixed narrow width for line numbers */
        background-color: #f7f7f7;
        color: #999;
        text-align: right;
        padding-right: 4px;
        border-right: 1px solid #eee;
        vertical-align: top;
    }
    
    /* Content Columns (2nd and 4th cols) */
    td {
        vertical-align: top;
        padding: 0 4px;
        
        /* These 3 properties are critical to override difflib's nowrap and force content to fit */
        white-space: pre-wrap; 
        word-wrap: break-word; 
        overflow-wrap: break-word;
    }
    
    /* Explicitly prevent content columns from collapsing or exploding */
    /* By default in fixed layout, remaining space is shared by columns without width. 
       Since diff_header has fixed width, these two will split the rest 50/50. */

    .diff_next { background-color: #c0c0c0; }
    .diff_add { background-color: #e6ffec; }
    .diff_chg { background-color: #ffffd7; }
    .diff_sub { background-color: #ffebe9; }
</style>
"""

# Inject custom CSS
html_diff = html_diff.replace('<head>', '<head>' + style_tag)

with open(output_path, 'w') as out:
    out.write(html_diff)

print(f"Diff generated at {output_path}")

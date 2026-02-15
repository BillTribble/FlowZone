import json
import os

ISSUE_FILE = ".beads/issues.jsonl"
PHASE_2_TASKS = ["bd-7wy", "bd-34z", "bd-n3n", "bd-1u3", "bd-338"]
PHASE_3_TASKS = ["bd-vtl", "bd-hdu"]
BAD_PHASE_2_PARENT = "bd-6mv"
BAD_PHASE_3_PARENT = "bd-2xf"

def fix_dependencies():
    if not os.path.exists(ISSUE_FILE):
        print(f"Error: {ISSUE_FILE} not found.")
        return

    new_lines = []
    modified_count = 0

    with open(ISSUE_FILE, 'r') as f:
        for line in f:
            try:
                issue = json.loads(line)
                issue_id = issue.get("id")
                
                if issue_id in PHASE_2_TASKS:
                    deps = issue.get("dependencies", [])
                    # Handle case where dep might be string or dict
                    new_deps = []
                    for d in deps:
                        if isinstance(d, dict):
                             if d.get("depends_on_id") != BAD_PHASE_2_PARENT:
                                 new_deps.append(d)
                        elif isinstance(d, str):
                             if d != BAD_PHASE_2_PARENT:
                                 new_deps.append(d)
                    
                    if len(deps) != len(new_deps):
                        issue["dependencies"] = new_deps
                        modified_count += 1
                        print(f"Fixed {issue_id}: Removed {BAD_PHASE_2_PARENT}")
                
                elif issue_id in PHASE_3_TASKS:
                    deps = issue.get("dependencies", [])
                    new_deps = []
                    for d in deps:
                        if isinstance(d, dict):
                             if d.get("depends_on_id") != BAD_PHASE_3_PARENT:
                                 new_deps.append(d)
                        elif isinstance(d, str):
                             if d != BAD_PHASE_3_PARENT:
                                 new_deps.append(d)

                    if len(deps) != len(new_deps):
                        issue["dependencies"] = new_deps
                        modified_count += 1
                        print(f"Fixed {issue_id}: Removed {BAD_PHASE_3_PARENT}")

                new_lines.append(json.dumps(issue) + "\n")
            except json.JSONDecodeError:
                new_lines.append(line)

    with open(ISSUE_FILE, 'w') as f:
        f.writelines(new_lines)
    
    print(f"Done. Modified {modified_count} issues.")

if __name__ == "__main__":
    fix_dependencies()

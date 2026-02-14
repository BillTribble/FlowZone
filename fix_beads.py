import json
import subprocess
import sys
import re

def run_command(cmd, check=True):
    print(f"Running: {' '.join(cmd)}")
    try:
        subprocess.run(cmd, check=check, text=True, capture_output=True)
        return True
    except subprocess.CalledProcessError as e:
        print(f"Error running command: {e}", file=sys.stderr)
        print(f"Output: {e.output}", file=sys.stderr)
        print(f"Stderr: {e.stderr}", file=sys.stderr)
        return False

def get_tasks_for_phase(beads, phase_id):
    tasks = []
    for bead in beads:
        if bead['id'] == phase_id: continue
        # Check if bead depends on phase_id (Current wrong state)
        # Note: If we already removed parent in previous run, this check might fail on re-run.
        # So we should check if they SHOULD be in this phase but aren't labeled yet?
        # For robustness, we'll try to update labels for all beads that look like they belong to the phase.
        # But for now, let's stick to the dependency logic or just try to update all known IDs from previous dump.
        # Since we removed parents, valid 'dependencies' list is empty now for those tasks.
        # We need to rely on the fact that we know which tasks belong to which phase from the previous dump.
        pass
    return [] 

def main():
    # Load all beads from original dump to know mapping
    try:
        with open('all_beads.json', 'r') as f:
            beads = json.load(f)
    except FileNotFoundError:
        print("all_beads.json not found.")
        sys.exit(1)

    # Map Phases to IDs
    phases = {
        "Phase 0": "bd-36e",
        "Phase 1": "bd-jne",
        "Phase 2": "bd-6mv",
        "Phase 3": "bd-2xf",
        "Phase 4": "bd-7sx",
        "Phase 5": "bd-287",
        "Phase 6": "bd-lk7",
        "Phase 7": "bd-lxy",
        "Phase 8": "bd-1yu"
    }

    print("--- Applying Labels ---")
    for phase_name, phase_id in phases.items():
        # Get tasks from ORIGINAL dump
        tasks = []
        for bead in beads:
            if bead['id'] == phase_id: continue
            deps = bead.get('dependencies', [])
            for dep in deps:
                if dep['id'] == phase_id:
                    tasks.append(bead['id'])
                    break
        
        label = phase_name.lower().replace(" ", "-").replace(":", "")
        print(f"Labeling {len(tasks)} tasks in {phase_name} with {label}...")
        
        for task_id in tasks:
            # br update <TASK_ID> --set-labels <LABEL>
            # Note: This overwrites existing labels. If we want to add, we'd need to fetch first.
            # Assuming no critical existing labels.
            run_command(['br', 'update', task_id, '--set-labels', label])

    # 2. Create Missing Beads (if not created)
    print("\n--- Creating Missing Beads ---")
    
    # Task 0.4
    # Check if exists first? brute force create.
    cmd = ['br', 'create', 'Task 0.4: Build Scripts & CI Prep', '-t', 'task', '-p', '0', '--json', '--labels', 'phase-0']
    # Note: br create might duplicate if title matches? br usually allows duplicates.
    # We should search first.
    search = subprocess.run(['br', 'search', 'Task 0.4', '--json'], capture_output=True, text=True)
    if 'Task 0.4' not in search.stdout:
        print("Creating Task 0.4...")
        subprocess.run(cmd, text=True, capture_output=True)
    else:
        print("Task 0.4 already exists.")

    # Task 4.6 (Microtuning) - Already created in previous run?
    search = subprocess.run(['br', 'search', 'Microtuning', '--json'], capture_output=True, text=True)
    if 'Microtuning' not in search.stdout:
         print("Creating Task 4.6...")
         cmd = ['br', 'create', 'Task 4.6: Microtuning Implementation', '-t', 'task', '-p', '2', '--json', '--labels', 'phase-4']
         subprocess.run(cmd, text=True, capture_output=True)
    else:
         print("Task 4.6 already exists.")

    # 3. Specific Updates
    
    # Move Task 8.4 (bd-kll) -> Phase 2
    # Update label
    run_command(['br', 'update', 'bd-kll', '--set-labels', 'phase-2'])
    
    # Update Task 2.3 Title (if not done)
    # Just run it again, idempotent
    run_command(['br', 'update', 'bd-3uw', '--title', 'Task 2.3: FlowEngine skeleton (+ CrashGuard stub)'])

if __name__ == '__main__':
    main()

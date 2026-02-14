import json
import subprocess
import re
import sys

def get_bead_ids(list_file):
    ids = []
    with open(list_file, 'r') as f:
        for line in f:
            match = re.search(r'bd-[a-z0-9]+', line)
            if match:
                ids.append(match.group(0))
    return ids

def fetch_bead_details(bead_id):
    try:
        result = subprocess.run(
            ['br', 'show', bead_id, '--json'],
            capture_output=True,
            text=True,
            check=True
        )
        data = json.loads(result.stdout)
        # br show returns a list, usually with one item
        if isinstance(data, list) and len(data) > 0:
            return data[0]
        return data
    except subprocess.CalledProcessError as e:
        print(f"Error fetching {bead_id}: {e}", file=sys.stderr)
        return None
    except json.JSONDecodeError as e:
        print(f"Error parsing JSON for {bead_id}: {e}", file=sys.stderr)
        return None

def main():
    list_file = 'beads_list.txt'
    output_file = 'all_beads.json'
    
    print(f"Reading IDs from {list_file}...")
    ids = get_bead_ids(list_file)
    print(f"Found {len(ids)} beads.")
    
    all_beads = []
    for i, bead_id in enumerate(ids):
        print(f"Fetching {bead_id} ({i+1}/{len(ids)})...")
        details = fetch_bead_details(bead_id)
        if details:
            all_beads.append(details)
            
    print(f"Saving {len(all_beads)} records to {output_file}...")
    with open(output_file, 'w') as f:
        json.dump(all_beads, f, indent=2)
    print("Done.")

if __name__ == '__main__':
    main()

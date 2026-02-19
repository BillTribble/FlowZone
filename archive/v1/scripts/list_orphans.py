import json
from pathlib import Path


def load_issues(path: Path) -> list[dict]:
    with path.open() as handle:
        return [json.loads(line) for line in handle if line.strip()]


def main() -> None:
    issues_path = Path('.beads/issues.jsonl')
    issues = load_issues(issues_path)

    orphans = []
    for issue in issues:
        if issue.get('issue_type') == 'epic':
            continue
        dependencies = issue.get('dependencies', [])
        if any(dep.get('type') == 'parent-child' for dep in dependencies):
            continue
        orphans.append(issue)

    print(f"Remaining orphan tasks: {len(orphans)}")
    for issue in orphans[:20]:
        labels = ','.join(issue.get('labels', [])) or '<no-label>'
        print(f"- {issue['id']}: {issue['title']} [labels: {labels}]")


if __name__ == '__main__':
    main()

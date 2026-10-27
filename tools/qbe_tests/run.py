#!/usr/bin/env python3

import argparse
import subprocess
import sys

from pathlib import Path

import tqdm

REPO_ROOT_ABS = Path(__file__).resolve().parents[2]
REPO_ROOT = REPO_ROOT_ABS.relative_to(Path.cwd())
QBE_REPO_URL = "git://c9x.me/qbe.git"
QBE_REPO_LOCAL_DIR_DEFAULT = REPO_ROOT / "tmp" / "qbe_repo"
QBE_TESTS_DIR_DEFAULT = REPO_ROOT / "tmp" / "qbe_tests"
SILVA_SYNTAX_DEFAULT = REPO_ROOT / "build.default.release" / "src" / "silva_syntax"
QBE_SEED_DEFAULT = REPO_ROOT / "src" / "zoo" / "qbe" / "qbe.seed"

TEST_GLOBS_QBE_REPO = ["test/*.ssa", "test/*.qbe"]
TEST_GLOB_QBE = "**/*.qbe"

# setup


def ensure_tests_repo(tests_repo_dir: Path) -> None:
    assert not tests_repo_dir.exists()
    print(f"Cloning {QBE_REPO_URL} into {tests_repo_dir} ...", flush=True)
    cmd = ["git", "clone", "--progress", QBE_REPO_URL, str(tests_repo_dir)]
    subprocess.run(
        cmd,
        check=True,
        stdout=sys.stdout,
        stderr=sys.stderr,
    )


# Silva's fragmentization does not allow tabs, so they are expanded to spaces.
def preprocess_all(tests_repo_dir: Path, qbe_tests_dir: Path) -> list[Path]:
    assert tests_repo_dir.exists()
    assert qbe_tests_dir.exists()

    src_files: list[Path] = []
    for glob in TEST_GLOBS_QBE_REPO:
        src_files.extend(tests_repo_dir.glob(glob))
    qbe_files: list[Path] = []
    for src_file in tqdm.tqdm(sorted(src_files)):
        rel = src_file.relative_to(tests_repo_dir)
        qbe_file = (qbe_tests_dir / rel).with_suffix(".qbe")
        qbe_file.parent.mkdir(parents=True, exist_ok=True)
        text = src_file.read_text()
        qbe_file.write_text("\n".join(line.expandtabs(8) for line in text.split("\n")))
        qbe_files.append(qbe_file)
    return qbe_files


def cmd_setup(args: argparse.Namespace) -> int:
    assert not args.tests_repo_dir.exists(), (
        f"directory already exists: {args.tests_repo_dir}"
    )
    assert not args.qbe_tests_dir.exists(), (
        f"directory already exists: {args.qbe_tests_dir}"
    )
    args.tests_repo_dir.parent.mkdir(parents=True, exist_ok=True)
    args.qbe_tests_dir.mkdir(parents=True, exist_ok=True)

    ensure_tests_repo(args.tests_repo_dir)
    qbe_files = preprocess_all(args.tests_repo_dir, args.qbe_tests_dir)
    print(f"Preprocessed {len(qbe_files)} files into {args.qbe_tests_dir}.")
    return 0


# run-tests


def cmd_run_tests(args: argparse.Namespace):
    assert args.qbe_tests_dir.exists(), f"no directory: {args.qbe_tests_dir}"

    if args.input_file_list:
        qbe_files = [Path(x) for x in args.input_file_list.read_text().split("\n") if x]
    else:
        qbe_files = sorted(args.qbe_tests_dir.glob(TEST_GLOB_QBE))
    assert len(qbe_files) >= 1
    if args.max_count:
        qbe_files = qbe_files[: args.max_count]

    failed_tests = []
    status = tqdm.tqdm(bar_format="{desc}", position=0)
    errors = tqdm.tqdm(bar_format="{desc}", position=1)
    progress = tqdm.tqdm(qbe_files, position=2)
    count = 0
    for qbe_file in progress:
        count += 1
        cmd = [
            str(args.silva_syntax),
            str(args.qbe_seed),
            str(qbe_file),
            "--action=none",
        ]
        status.set_description_str(str(qbe_file))
        result = subprocess.run(
            cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL
        )
        if result.returncode != 0:
            failed_tests.append(qbe_file)
        errors.set_description_str(
            f"Failed {len(failed_tests)} out of {count} ({len(failed_tests) / count * 100.0:.2f}%)"
        )

    if failed_tests:
        if args.output_file_list:
            args.output_file_list.write_text(
                "\n".join((str(x) for x in failed_tests)) + "\n"
            )
    return 0


# main


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers(dest="command", required=True)

    p_setup = subparsers.add_parser("setup")
    p_setup.add_argument(
        "--tests-repo-dir", type=Path, default=QBE_REPO_LOCAL_DIR_DEFAULT
    )
    p_setup.add_argument("--qbe-tests-dir", type=Path, default=QBE_TESTS_DIR_DEFAULT)
    p_setup.set_defaults(func=cmd_setup)

    p_run_tests = subparsers.add_parser("run-tests")
    p_run_tests.add_argument("--qbe-tests-dir", type=Path, default=QBE_TESTS_DIR_DEFAULT)
    p_run_tests.add_argument("--silva-syntax", type=Path, default=SILVA_SYNTAX_DEFAULT)
    p_run_tests.add_argument("--qbe-seed", type=Path, default=QBE_SEED_DEFAULT)
    p_run_tests.add_argument("--max-count", type=int)
    p_run_tests.add_argument("--input-file-list", type=Path)
    p_run_tests.add_argument("--output-file-list", type=Path)
    p_run_tests.set_defaults(func=cmd_run_tests)

    return parser.parse_args()


def main() -> int:
    args = parse_args()
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())

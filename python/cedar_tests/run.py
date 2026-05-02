#!/usr/bin/env python3

import argparse
import subprocess
import sys

from pathlib import Path

import tqdm

REPO_ROOT_ABS = Path(__file__).resolve().parents[2]
REPO_ROOT = REPO_ROOT_ABS.relative_to(Path.cwd())
WACCT_REPO_URL = "https://github.com/nlsandler/writing-a-c-compiler-tests.git"
WACCT_REPO_LOCAL_DIR_DEFAULT = REPO_ROOT / "var" / "wacct"
CEDAR_TESTS_DIR_DEFAULT = REPO_ROOT / "var" / "cedar-tests"
SILVA_CEDAR_DEFAULT = REPO_ROOT / "build" / "cpp" / "silva_cedar"

CHAPTER_GLOB_C = "chapter_*/valid/**/*.c"
CHAPTER_GLOB_CEDAR = "**/*.cedar"

# setup


def ensure_tests_repo(tests_repo_dir: Path) -> None:
    assert not tests_repo_dir.exists()
    print(f"Cloning {WACCT_REPO_URL} into {tests_repo_dir} ...", flush=True)
    cmd = ["git", "clone", "--progress", WACCT_REPO_URL, str(tests_repo_dir)]
    subprocess.run(
        cmd,
        check=True,
        stdout=sys.stdout,
        stderr=sys.stderr,
    )


def preprocess_all(tests_repo_dir: Path, cedar_tests_dir: Path) -> list[Path]:
    assert tests_repo_dir.exists()
    assert cedar_tests_dir.exists()

    src_root = tests_repo_dir / "tests"
    cedar_files: list[Path] = []
    for c_file in tqdm.tqdm(sorted(src_root.glob(CHAPTER_GLOB_C))):
        rel = c_file.relative_to(src_root)
        cedar_file = (cedar_tests_dir / rel).with_suffix(".cedar")
        cedar_file.parent.mkdir(parents=True, exist_ok=True)
        cmd = ["gcc", "-E", str(c_file), "-o", str(cedar_file)]
        subprocess.run(
            cmd,
            check=True,
            stdout=sys.stdout,
            stderr=sys.stderr,
        )
        cedar_files.append(cedar_file)
    return cedar_files


def cmd_setup(args: argparse.Namespace) -> int:
    assert not args.tests_repo_dir.exists(), f'directory already exists: {args.tests_repo_dir}'
    assert not args.cedar_tests_dir.exists(), f'directory already exists: {args.cedar_tests_dir}'
    args.tests_repo_dir.parent.mkdir(parents=True, exist_ok=True)
    args.cedar_tests_dir.mkdir(parents=True, exist_ok=True)

    ensure_tests_repo(args.tests_repo_dir)
    cedar_files = preprocess_all(args.tests_repo_dir, args.cedar_tests_dir)
    print(f"Preprocessed {len(cedar_files)} files into {args.cedar_tests_dir}.")
    return 0


# run-tests


def cmd_run_tests(args: argparse.Namespace):
    assert args.cedar_tests_dir.exists(), f'no directory: {args.cedar_tests_dir}'

    if args.input_file_list:
        cedar_files = args.input_file_list.read_text().split('\n')
    else:
        cedar_files = sorted(args.cedar_tests_dir.glob(CHAPTER_GLOB_CEDAR))
    assert len(cedar_files) >= 1
    if args.max_count:
        cedar_files = cedar_files[:args.max_count]

    failed_tests = []
    status = tqdm.tqdm(bar_format='{desc}', position=0)
    errors = tqdm.tqdm(bar_format='{desc}', position=1)
    progress = tqdm.tqdm(cedar_files, position=2)
    count = 0
    for cedar_file in progress:
        count += 1
        cmd = [str(args.silva_cedar), str(cedar_file)]
        status.set_description_str(str(cedar_file))
        result = subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        if result.returncode != 0:
            failed_tests.append(cedar_file)
        errors.set_description_str(
            f"Failed {len(failed_tests)} out of {count} ({len(failed_tests) / count * 100.0:.2f}%)"
        )

    if failed_tests:
        if args.output_file_list:
            args.output_file_list.write_text('\n'.join((str(x) for x in failed_tests)))
        return 1


# main


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers(dest="command", required=True)

    p_setup = subparsers.add_parser("setup")
    p_setup.add_argument("--tests-repo-dir", type=Path, default=WACCT_REPO_LOCAL_DIR_DEFAULT)
    p_setup.add_argument("--cedar-tests-dir", type=Path, default=CEDAR_TESTS_DIR_DEFAULT)
    p_setup.set_defaults(func=cmd_setup)

    p_run_tests = subparsers.add_parser("run-tests")
    p_run_tests.add_argument("--cedar-tests-dir", type=Path, default=CEDAR_TESTS_DIR_DEFAULT)
    p_run_tests.add_argument("--silva-cedar", type=Path, default=SILVA_CEDAR_DEFAULT)
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

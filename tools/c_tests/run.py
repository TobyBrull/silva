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
C_TESTS_DIR_DEFAULT = REPO_ROOT / "var" / "c_tests"
SILVA_SYNTAX_DEFAULT = REPO_ROOT / "build.default.release" / "cpp" / "silva_syntax"
C_SEED_DEFAULT = REPO_ROOT / "cpp" / "zoo" / "c" / "c.seed"

CHAPTER_GLOB_WACCT = "chapter_*/valid/**/*.c"
CHAPTER_GLOB_C = "**/*.c"

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


def preprocess_all(tests_repo_dir: Path, c_tests_dir: Path) -> list[Path]:
    assert tests_repo_dir.exists()
    assert c_tests_dir.exists()

    src_root = tests_repo_dir / "tests"
    c_files: list[Path] = []
    for wacct_file in tqdm.tqdm(sorted(src_root.glob(CHAPTER_GLOB_WACCT))):
        rel = wacct_file.relative_to(src_root)
        c_file = (c_tests_dir / rel).with_suffix(".c")
        c_file.parent.mkdir(parents=True, exist_ok=True)
        cmd = ["gcc", "-E", str(wacct_file), "-o", str(c_file)]
        subprocess.run(
            cmd,
            check=True,
            stdout=sys.stdout,
            stderr=sys.stderr,
        )
        c_files.append(c_file)
    return c_files


def cmd_setup(args: argparse.Namespace) -> int:
    assert not args.tests_repo_dir.exists(), (
        f"directory already exists: {args.tests_repo_dir}"
    )
    assert not args.c_tests_dir.exists(), (
        f"directory already exists: {args.c_tests_dir}"
    )
    args.tests_repo_dir.parent.mkdir(parents=True, exist_ok=True)
    args.c_tests_dir.mkdir(parents=True, exist_ok=True)

    ensure_tests_repo(args.tests_repo_dir)
    c_files = preprocess_all(args.tests_repo_dir, args.c_tests_dir)
    print(f"Preprocessed {len(c_files)} files into {args.c_tests_dir}.")
    return 0


# run-tests


def cmd_run_tests(args: argparse.Namespace):
    assert args.c_tests_dir.exists(), f"no directory: {args.c_tests_dir}"

    if args.input_file_list:
        c_files = [
            Path(x) for x in args.input_file_list.read_text().split("\n") if x
        ]
    else:
        c_files = sorted(args.c_tests_dir.glob(CHAPTER_GLOB_C))
    assert len(c_files) >= 1
    if args.max_count:
        c_files = c_files[: args.max_count]

    failed_tests = []
    status = tqdm.tqdm(bar_format="{desc}", position=0)
    errors = tqdm.tqdm(bar_format="{desc}", position=1)
    progress = tqdm.tqdm(c_files, position=2)
    count = 0
    for c_file in progress:
        count += 1
        cmd = [
            str(args.silva_syntax),
            str(args.c_seed),
            str(c_file),
            "--action=none",
        ]
        status.set_description_str(str(c_file))
        result = subprocess.run(
            cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL
        )
        if result.returncode != 0:
            failed_tests.append(c_file)
        errors.set_description_str(
            f"Failed {len(failed_tests)} out of {count} ({len(failed_tests) / count * 100.0:.2f}%)"
        )

    if failed_tests:
        if args.output_file_list:
            args.output_file_list.write_text("\n".join((str(x) for x in failed_tests)))
        return 1
    return 0


# main


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers(dest="command", required=True)

    p_setup = subparsers.add_parser("setup")
    p_setup.add_argument(
        "--tests-repo-dir", type=Path, default=WACCT_REPO_LOCAL_DIR_DEFAULT
    )
    p_setup.add_argument(
        "--c-tests-dir", type=Path, default=C_TESTS_DIR_DEFAULT
    )
    p_setup.set_defaults(func=cmd_setup)

    p_run_tests = subparsers.add_parser("run-tests")
    p_run_tests.add_argument(
        "--c-tests-dir", type=Path, default=C_TESTS_DIR_DEFAULT
    )
    p_run_tests.add_argument("--silva-syntax", type=Path, default=SILVA_SYNTAX_DEFAULT)
    p_run_tests.add_argument("--c-seed", type=Path, default=C_SEED_DEFAULT)
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

#!/usr/bin/env python3

import argparse
import subprocess
import sys

from collections.abc import Callable
from dataclasses import dataclass
from pathlib import Path

import tqdm

REPO_ROOT_ABS = Path(__file__).resolve().parents[2]
REPO_ROOT = REPO_ROOT_ABS.relative_to(Path.cwd())

# languages


def preprocess_c(src_file: Path, dst_file: Path) -> None:
    cmd = ["gcc", "-E", str(src_file), "-o", str(dst_file)]
    subprocess.run(
        cmd,
        check=True,
        stdout=sys.stdout,
        stderr=sys.stderr,
    )


# Silva's fragmentization does not allow tabs, so they are expanded to spaces.
def preprocess_qbe(src_file: Path, dst_file: Path) -> None:
    text = src_file.read_text()
    dst_file.write_text("\n".join(line.expandtabs(8) for line in text.split("\n")))


@dataclass(frozen=True)
class TestLanguage:
    name: str
    repo_url: str
    repo_globs: list[str]
    seed: Path
    suffix: str
    preprocess: Callable[[Path, Path], None]
    max_failures: int

    def repo_dir(self, work_dir: Path) -> Path:
        return work_dir / self.name / "repo"

    def tests_dir(self, work_dir: Path) -> Path:
        return work_dir / self.name / "tests"

    def failed_file(self, work_dir: Path) -> Path:
        return work_dir / self.name / "failed.txt"


LANGUAGES = {
    x.name: x
    for x in [
        TestLanguage(
            name="c",
            repo_url="https://github.com/nlsandler/writing-a-c-compiler-tests.git",
            repo_globs=["tests/chapter_*/valid/**/*.c"],
            seed=REPO_ROOT / "src" / "zoo" / "c" / "c.seed",
            suffix=".c",
            preprocess=preprocess_c,
            max_failures=3,
        ),
        TestLanguage(
            name="qbe",
            repo_url="git://c9x.me/qbe.git",
            repo_globs=["test/*.ssa", "test/*.qbe"],
            seed=REPO_ROOT / "src" / "zoo" / "qbe" / "qbe.seed",
            suffix=".qbe",
            preprocess=preprocess_qbe,
            max_failures=0,
        ),
    ]
}

# setup


def ensure_tests_repo(lang: TestLanguage, work_dir: Path) -> None:
    repo_dir = lang.repo_dir(work_dir)
    assert not repo_dir.exists()
    print(f"Cloning {lang.repo_url} into {repo_dir} ...", flush=True)
    cmd = ["git", "clone", "--progress", lang.repo_url, str(repo_dir)]
    subprocess.run(
        cmd,
        check=True,
        stdout=sys.stdout,
        stderr=sys.stderr,
    )


def preprocess_all(lang: TestLanguage, work_dir: Path) -> list[Path]:
    repo_dir = lang.repo_dir(work_dir)
    tests_dir = lang.tests_dir(work_dir)
    assert repo_dir.exists()
    assert tests_dir.exists()

    src_files: list[Path] = []
    for glob in lang.repo_globs:
        src_files.extend(repo_dir.glob(glob))
    test_files: list[Path] = []
    for src_file in tqdm.tqdm(sorted(src_files)):
        rel = src_file.relative_to(repo_dir)
        test_file = (tests_dir / rel).with_suffix(lang.suffix)
        test_file.parent.mkdir(parents=True, exist_ok=True)
        lang.preprocess(src_file, test_file)
        test_files.append(test_file)
    return test_files


def cmd_setup(language: TestLanguage, args: argparse.Namespace) -> int:
    repo_dir = language.repo_dir(args.work_dir)
    tests_dir = language.tests_dir(args.work_dir)
    assert not repo_dir.exists(), f"directory already exists: {repo_dir}"
    assert not tests_dir.exists(), f"directory already exists: {tests_dir}"
    repo_dir.parent.mkdir(parents=True, exist_ok=True)
    tests_dir.mkdir(parents=True, exist_ok=True)

    ensure_tests_repo(language, args.work_dir)
    test_files = preprocess_all(language, args.work_dir)
    print(f"Preprocessed {len(test_files)} files into {tests_dir}.")
    return 0


# run-tests


def cmd_run_tests(language: TestLanguage, args: argparse.Namespace):
    tests_dir = language.tests_dir(args.work_dir)
    assert tests_dir.exists(), f"no directory: {tests_dir}"

    test_files = sorted(tests_dir.glob(f"**/*{language.suffix}"))
    assert len(test_files) >= 1

    failed_tests = []
    status = tqdm.tqdm(bar_format="{desc}", position=0)
    errors = tqdm.tqdm(bar_format="{desc}", position=1)
    progress = tqdm.tqdm(test_files, position=2)
    count = 0
    for test_file in progress:
        count += 1
        cmd = [
            str(args.silva_syntax_exe),
            str(language.seed),
            str(test_file),
            "--action=none",
        ]
        status.set_description_str(str(test_file))
        result = subprocess.run(
            cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL
        )
        if result.returncode != 0:
            failed_tests.append(test_file)
        errors.set_description_str(
            f"Failed {len(failed_tests)} out of {count} ({len(failed_tests) / count * 100.0:.2f}%)"
        )

    failed_file = language.failed_file(args.work_dir)
    failed_file.write_text("".join(f"{x}\n" for x in failed_tests))
    assert len(failed_tests) <= language.max_failures, (
        f"More than {language.max_failures} {language.name} tests failed, see {failed_file}"
    )
    return 0


# main


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--language", required=True, choices=sorted(LANGUAGES))
    parser.add_argument("--work-dir", required=True, type=Path)
    subparsers = parser.add_subparsers(dest="command", required=True)

    p_setup = subparsers.add_parser("setup")
    p_setup.set_defaults(func=cmd_setup)

    p_run_tests = subparsers.add_parser("run-tests")
    p_run_tests.add_argument(
        "--silva-syntax-exe",
        type=Path,
        default=REPO_ROOT / "build.default.release" / "src" / "silva_syntax",
    )
    p_run_tests.set_defaults(func=cmd_run_tests)

    args = parser.parse_args()
    return args


def main() -> int:
    args = parse_args()
    return args.func(LANGUAGES[args.language], args)


if __name__ == "__main__":
    sys.exit(main())

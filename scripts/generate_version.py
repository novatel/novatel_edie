import argparse
from pathlib import Path

import gitlab
import semver
from nov_gitlab import scrape_commit, release


def generate_new_version_file(version_file: str, increment_type: str, out_dir: str):
    """Parses the version string in version file and increments the appropriate number, and returns the new string

    Args:
        version_file: Path of the version file
        increment_type: What version number to increment (major, minor, patch)
        out_dir: Path to put the new version.txt

    Returns:
        new_ver and Str of the new version file as tuple
    """
    with open(version_file, 'r+') as vf:
        version_contents = vf.read()

    lines = version_contents.splitlines()
    new_version_file_contents = ''
    current_version = ''

    for line in lines:
        if line.startswith('#') and line.find('RELEASE_VERSION') != -1:
            _, _, current_version = line.split(' ')
            current_version = semver.VersionInfo.parse(current_version.strip("\""))
            increment_type = increment_type.lower()
            if increment_type == 'patch':
                current_version = current_version.bump_patch()
            elif increment_type == 'minor':
                current_version = current_version.bump_minor()
            elif increment_type == 'major':
                current_version = current_version.bump_major()
            else:
                raise ValueError('Incorrect version type. Options are major, minor, or patch')
            line = f'#define RELEASE_VERSION \"{current_version}\"'

        new_version_file_contents += f'{line}\n'

    with open(f'{out_dir}/version.txt', 'w') as new_version_file:
        new_version_file.write(str(current_version))

    with open(version_file, 'w') as vf:
        vf.write(new_version_file_contents)

    return current_version, new_version_file


def get_version_level(git_server: str, git_token: str, project_id: int, git_hash: str):
    """Basic function to get version level from either commit message or merge request

    Args:
        git_server: URL of the Git server
        git_token: Access token with permission to write to the Git repo
        project_id: Project id
        git_hash: SHA-1 hash of git commit

    Returns:
        version_level: major|minor|patch
    """
    gl = gitlab.Gitlab(git_server, private_token=git_token)

    project = scrape_commit.get_project(gl, project_id)
    commit = scrape_commit.get_commit(project, git_hash)
    version_level = scrape_commit.get_version_level_from_commit(commit)

    if version_level is None and scrape_commit.is_mr(commit):
        mr = scrape_commit.get_mr_from_commit(project, git_hash)
        # Falls through to default level of patch if it can't find an MR
        if mr is None:
            print("Failed to find link to merge request in description.")
        else:
            version_level = scrape_commit.get_version_level_from_mr(mr)
    
    if version_level is None:
        print("Unable to find version level, defaulting to 'patch'")
        version_level = 'patch'
    
    return version_level


def get_new_version_file(version_file: str, git_server: str, git_token: str, project_id: int, git_hash: str,
                         out_dir: str):
    """Basic method to create version level and new updated file
    Args:
        version_file: Path to version file
        git_server: URL of the Git server
        git_token: Access token with permission to write to the Git repo
        project_id: Project id
        git_hash: SHA-1 hash of git commit
        out_dir: Path to put the new version.txt

    Returns:
        version: incremented version string
        new_version_file: new version file with updated version
    """
    version_level = get_version_level(git_server, git_token, project_id, git_hash)
    version, new_version_file = generate_new_version_file(version_file, version_level, out_dir)
    return version, new_version_file


def parse_args():
    p = argparse.ArgumentParser()
    p.add_argument('path', help='Path to version file')
    p.add_argument('--sha', help='New Git SHA to append to the version file')
    p.add_argument('--git-server', help='Git server address')
    p.add_argument('--git-token', help='Git deployment token')
    p.add_argument('--git-project_id', help='Project id integer', type=int)
    p.add_argument('--git-branch', help='Which branch to submit the new version file to', default='master')
    p.add_argument('--output-path', help='Path to output the new version file')
    p.add_argument('--get_new_version_file', help='Calls get_new_version_file()', action='store_true', default=False)
    p.add_argument('--push_version', help='Calls push_version()', action='store_true', default=False)
    args = p.parse_args()

    if args.git_server or args.git_token or args.git_project_id:
        if not (args.git_server and args.git_token and args.git_project_id):
            p.error('If one git_* argument is set they all must be')
    return args


def main():
    args = parse_args()
    version_path = args.path
    ci_server_url = args.git_server
    git_token = args.git_token
    git_project_id = args.git_project_id

    if args.get_new_version_file:
        git_commit_sha = args.sha
        get_new_version_file(version_path, ci_server_url, git_token, git_project_id, git_commit_sha,
                             args.output_path)
        print("get_new_version_file arguments:", version_path, ci_server_url, git_project_id, git_commit_sha,
              version_path)

    if args.push_version:
        git_branch = args.git_branch

        with open(version_path) as fp:
            pv_return = release.push_version(version_path, ci_server_url, git_token, git_project_id, git_branch,
                                             fp.read())
        print("push_version arguments:", version_path, ci_server_url, git_project_id, git_branch, version_path,
              pv_return)


if __name__ == '__main__':
    main()

#!/bin/bash
#
# Copyright (c) 2026 Rockchip Electronics Co., Ltd
#
# SPDX-License-Identifier: GPL-2.0
#

set -euo pipefail

usage()
{
	cat <<EOF
Usage:
  $0 -b <src-branch> <dst-branch> {-c <range-start> [range-end] | -n count} [-f path...] [-a author...] [-n count] [-o lineType]

Options:
  -b  branches: source branch and destination branch
  -c  source commit range: <range-start>..<range-end>, default range-end is <src-branch>
  -n  max output count after -c/-f/-a filters matched; required when -c is omitted
  -f  source pathspecs, supports multiple paths and !exclude pathspecs
  -a  source authors, supports multiple authors
  -o  output line type: + only shows "+ ..." lines, - only shows "- ..." lines

Output marks:
  -  same Change-Id exists in <dst-branch>
  +  Change-Id does not exist in <dst-branch>
  ?  source commit has no Change-Id

Example:
  $0 -b origin/next-dev origin/v5 -c 1f824d07f58 -f arch/arm/mach-rockchip/ lib/
  $0 -b origin/next-dev origin/v5 -c 1f824d07f58 6408eeee7c2 -a chenjh@rock-chips.com -n 100
  $0 -b origin/next-dev origin/v5 -n 100 -a chenjh@rock-chips.com
  $0 -b origin/next-dev origin/v5 -c 1f824d07f58 -f . '!drivers/' '!README.md'

EOF
}

is_option()
{
	case "$1" in
	-b|-c|-f|-a|-n|-o|-h|--help)
		return 0
		;;
	*)
		return 1
		;;
	esac
}

validate_author()
{
	case "$1" in
	*/*|!*|.)
		if [ -e "$1" ] || [ -e "${1#!}" ]; then
			echo "error: invalid author '$1'; did you forget -f before path '$1'?" >&2
		else
			echo "error: invalid author '$1'" >&2
		fi
		exit 2
		;;
	*@*)
		return 0
		;;
	*)
		echo "error: invalid author '$1'; expected email-like author, for example name@domain" >&2
		exit 2
		;;
	esac
}

if [ "$#" -eq 0 ]; then
	usage >&2
	exit 2
fi

src_branch=
dst_branch=
range_start=
range_end=
max_count=
line_type=
authors=()
pathspecs=()

while [ "$#" -gt 0 ]; do
	case "$1" in
	-b)
		if [ "$#" -lt 3 ] || is_option "$2" || is_option "$3"; then
			echo "error: -b requires <src-branch> <dst-branch>" >&2
			exit 2
		fi
		src_branch=$2
		dst_branch=$3
		shift 3
		;;
	-c)
		shift
		if [ "$#" -eq 0 ] || is_option "$1"; then
			echo "error: -c requires <range-start>" >&2
			exit 2
		fi
		range_start=$1
		shift
		if [ "$#" -gt 0 ] && ! is_option "$1"; then
			range_end=$1
			shift
		fi
		;;
	-f)
		shift
		if [ "$#" -eq 0 ] || is_option "$1"; then
			echo "error: -f requires at least one path" >&2
			exit 2
		fi
		while [ "$#" -gt 0 ] && ! is_option "$1"; do
			pathspecs+=("$1")
			shift
		done
		;;
	-a)
		shift
		if [ "$#" -eq 0 ] || is_option "$1"; then
			echo "error: -a requires at least one author" >&2
			exit 2
		fi
		while [ "$#" -gt 0 ] && ! is_option "$1"; do
			validate_author "$1"
			authors+=("$1")
			shift
		done
		;;
	-n)
		if [ "$#" -lt 2 ] || is_option "$2"; then
			echo "error: -n requires <count>" >&2
			exit 2
		fi
		case "$2" in
		*[!0-9]*|'')
			echo "error: invalid -n count: $2" >&2
			exit 2
			;;
		esac
		if [ "$2" -eq 0 ]; then
			echo "error: -n count must be greater than 0" >&2
			exit 2
		fi
		max_count=$2
		shift 2
		;;
	-o)
		if [ "$#" -lt 2 ]; then
			echo "error: -o requires <lineType>" >&2
			exit 2
		fi
		case "$2" in
		+|-)
			line_type=$2
			;;
		*)
			echo "error: invalid -o lineType: $2; expected + or -" >&2
			exit 2
			;;
		esac
		shift 2
		;;
	-h|--help)
		usage
		exit 0
		;;
	*)
		echo "error: unknown option or argument: $1" >&2
		usage >&2
		exit 2
		;;
	esac
done

if [ -z "$src_branch" ] || [ -z "$dst_branch" ]; then
	echo "error: missing -b <src-branch> <dst-branch>" >&2
	exit 2
fi

if [ -z "$range_start" ] && [ -z "$max_count" ]; then
	echo "error: either -c <range-start> or -n <count> is required" >&2
	exit 2
fi

tmpdir=$(mktemp -d)

trap 'rm -rf "$tmpdir"' EXIT

color_new=
color_reset=
if [ -t 1 ] && [ -z "${NO_COLOR:-}" ]; then
	color_new=$'\033[32m'
	color_reset=$'\033[m'
fi

git rev-parse --verify --quiet "${src_branch}^{commit}" >/dev/null || {
	echo "error: invalid source branch or commit: ${src_branch}" >&2
	exit 1
}

git rev-parse --verify --quiet "${dst_branch}^{commit}" >/dev/null || {
	echo "error: invalid destination branch or commit: ${dst_branch}" >&2
	exit 1
}

git log "$dst_branch" --format='%H%x09%(trailers:key=Change-Id,valueonly)' |
	awk -F '\t' 'NF >= 2 && $1 != "" && $2 != "" { print $2 "\t" $1 }' \
		>"$tmpdir/upstream-change-ids"

if [ -n "$range_start" ]; then
	git rev-parse --verify --quiet "${range_start}^{commit}" >/dev/null || {
		echo "error: invalid range start commit: ${range_start}" >&2
		exit 1
	}

	git merge-base --is-ancestor "$range_start" "$src_branch" || {
		echo "error: start commit ${range_start} is not reachable from ${src_branch}." >&2
		exit 1
	}

	if [ -z "$range_end" ]; then
		range_end=$src_branch
	fi

	git rev-parse --verify --quiet "${range_end}^{commit}" >/dev/null || {
		echo "error: invalid range end commit: ${range_end}" >&2
		exit 1
	}

	git merge-base --is-ancestor "$range_end" "$src_branch" || {
		echo "error: end commit ${range_end} is not reachable from ${src_branch}." >&2
		exit 1
	}

	git merge-base --is-ancestor "$range_start" "$range_end" || {
		echo "error: start commit ${range_start} is not an ancestor of end commit ${range_end}." >&2
		exit 1
	}

	commit_args=("${range_start}..${range_end}")
else
	commit_args=("$src_branch")
fi

rev_filters=()
for author in "${authors[@]}"; do
	rev_filters+=("--author=$author")
done

git_pathspecs=()
for pathspec in "${pathspecs[@]}"; do
	case "$pathspec" in
	!*)
		git_pathspecs+=(":(exclude)${pathspec#!}")
		;;
	*)
		git_pathspecs+=("$pathspec")
		;;
	esac
done

if [ "${#git_pathspecs[@]}" -gt 0 ]; then
	commits=$(git rev-list --reverse "${rev_filters[@]}" "${commit_args[@]}" -- "${git_pathspecs[@]}")
else
	commits=$(git rev-list --reverse "${rev_filters[@]}" "${commit_args[@]}")
fi

if [ -n "$max_count" ]; then
	commits=$(printf '%s\n' $commits | tail -n "$max_count")
fi

for commit in $commits; do
	short_commit=$(git rev-parse --short=11 "$commit")
	change_id=$(git log -1 --format='%(trailers:key=Change-Id,valueonly)' "$commit" |
		sed '/^[[:space:]]*$/d' |
		tail -n1)
	subject=$(git log -1 --format=%s "$commit")

	if [ -z "$change_id" ]; then
		if [ -n "$line_type" ]; then
			continue
		fi
		printf '? %s no-change-id %s\n' "$short_commit" "$subject"
		continue
	fi
	short_change_id=${change_id:0:8}

	match=$(awk -v change_id="$change_id" -F '\t' '$1 == change_id { print $2; exit }' "$tmpdir/upstream-change-ids")
	if [ -n "$match" ]; then
		if [ "$line_type" = "+" ]; then
			continue
		fi
		short_match=$(git rev-parse --short=11 "$match")
		printf -- '- %s | %s -> %s %s\n' "$short_change_id" "$short_commit" "$short_match" "$subject"
	else
		if [ "$line_type" = "-" ]; then
			continue
		fi
		printf '%s+ %s | %s %s%s\n' "$color_new" "$short_change_id" "$short_commit" "$subject" "$color_reset"
	fi
done

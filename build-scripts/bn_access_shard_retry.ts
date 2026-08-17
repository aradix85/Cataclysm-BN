/**
 * @module
 *
 * One second chance for a test shard that failed, for the fork's build.
 *
 * Upstream's suite shares global systems between cases, so a case can be decided by what ran
 * before it in the same process -- their own umbrella issue #3146. On a shard that means a
 * failure that has nothing to do with the commit under test, and it costs the whole run: the
 * packaged build is published only after the tests pass, so the rolling release goes stale
 * while nothing is actually broken.
 *
 * A failed shard is therefore run once more, in a fresh process with a fresh seed. A flaky
 * failure passes the second time and the run goes green without a human in the loop; a real
 * failure fails both times and still fails the build. The retry is never silent -- both
 * outcomes are announced in the log, so a shard that keeps needing one is visible rather
 * than quietly tolerated.
 *
 * This is a file of the fork's own, so merging upstream cannot conflict with it, and the
 * change in their shard runner stays an import and a wrapped call.
 */

/** Runs one shard and answers its exit status. The number says which attempt this is. */
export type ShardAttempt = (attempt: number) => Promise<number>

/**
 * Runs `attempt` and, when it fails, runs it exactly once more. Answers the status of the
 * last run, so a shard that fails twice still fails the build.
 */
export const runWithRetry = async (
  name: string,
  attempt: ShardAttempt,
  log: (message: string) => void = console.log,
): Promise<number> => {
  const first = await attempt(1)
  if (first === 0) {
    return first
  }
  log(`Shard ${name} failed with status ${first}; running it once more`)
  const second = await attempt(2)
  log(
    second === 0
      ? `Shard ${name} passed on its second run; its first failure was flaky`
      : `Shard ${name} failed twice, with status ${second}; the failure stands`,
  )
  return second
}

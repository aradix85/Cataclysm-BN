import { assertEquals } from "@std/assert"
import { runWithRetry } from "./bn_access_shard_retry.ts"

/** Runs the shard the given statuses in order, and records what was said about it. */
const recorder = (statuses: number[]) => {
  const attempts: number[] = []
  const said: string[] = []
  const attempt = (attempt: number) => {
    attempts.push(attempt)
    return Promise.resolve(statuses[attempts.length - 1])
  }
  return { attempts, said, attempt, log: (message: string) => said.push(message) }
}

Deno.test("a shard that passes is run once and says nothing about a retry", async () => {
  const r = recorder([0])
  assertEquals(await runWithRetry("10-cpu", r.attempt, r.log), 0)
  assertEquals(r.attempts, [1])
  assertEquals(r.said, [])
})

Deno.test("a shard that fails once is run again and the run goes green", async () => {
  const r = recorder([1, 0])
  assertEquals(await runWithRetry("10-cpu", r.attempt, r.log), 0)
  assertEquals(r.attempts, [1, 2])
  assertEquals(r.said.length, 2)
  assertEquals(r.said[1].includes("flaky"), true)
})

Deno.test("a shard that fails twice fails the build, and is not run a third time", async () => {
  const r = recorder([1, 3])
  assertEquals(await runWithRetry("10-cpu", r.attempt, r.log), 3)
  assertEquals(r.attempts, [1, 2])
  assertEquals(r.said[1].includes("stands"), true)
})

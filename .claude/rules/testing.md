# Testing

## Philosophy

- Tests verify behavior, not implementation — test what it does, not how
- A failing test is more valuable than a passing test that doesn't catch bugs
- Don't test framework code or obvious getters/setters
- Tests should be fast, isolated, and repeatable

## Coverage

- Unit test pure logic and domain rules
- Integration test boundaries (database, APIs, file system) with real infrastructure when practical
- E2E test critical user journeys only — keep these minimal
- Avoid mocking what you don't own; use real implementations or contract tests at integration boundaries

## Test Structure

- One assertion per test where practical — makes failures self-documenting
- Arrange / Act / Assert structure
- Test file mirrors source file structure
- Test names describe the scenario: `it('returns 404 when user does not exist')`

## What to Test

- Happy path
- Edge cases and boundary values
- Error conditions
- Anything that has broken in production

## What NOT to Test

- Private implementation details
- Third-party library behavior
- Trivial code that cannot meaningfully fail

## Running Tests

Tests should be runnable with a single command. Document that command in CLAUDE.md.
Never skip tests before committing. Fix failures or explicitly defer them with a tracked issue.

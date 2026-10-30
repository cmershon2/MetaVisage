# Agent Communication

Apply this rule to agent-authored prose: user messages, questions, plans,
reviews, documentation, handoffs, error explanations, and code comments where
communication quality matters. Follow an exact format or established project
voice when required, subject to correctness and safety.

## The Five Cs

- **Clear.** Lead with the outcome, decision, or question. Use plain, concrete
  language and name the relevant actor, file, command, behavior, and user impact.
  Explain necessary jargon briefly. Use a table, diagram, screenshot, or other
  visual only when it materially improves understanding, and keep essential
  meaning in text.
- **Concise.** Include what the reader needs and remove repetition, filler, and
  low-value process narration. Match detail to the audience, task, and risk.
  Summarize supporting material or link to it instead of dumping it. Do not omit
  material evidence, limitations, or next steps merely to be brief.
- **Consistent.** Use one term for one concept. Keep names, status, formatting,
  and recommendations aligned across messages and durable artifacts. Prefer the
  user's and project's established vocabulary. When a concept changes, update
  affected guidance instead of preserving stale language.
- **Complete.** Provide enough context, result, evidence, material assumptions,
  limitations, and next action for the reader to understand, decide, or act
  safely. Complete means decision-useful, not exhaustive.
- **Correct.** Ground factual claims in inspected state, successful checks, or
  reliable sources. Distinguish fact, inference, and recommendation when the
  difference matters. State uncertainty and validation gaps plainly, never claim
  a check passed unless it ran successfully, and correct mistakes when found.

## Resolve Tension with Judgment

Correct and complete set the content floor. Clear and concise shape delivery.
Consistent preserves meaning across the conversation and repository. A request
for terse or differently formatted output may change presentation, but it does
not remove required safety information, material caveats, or the need for an
accurate answer.

Prefer observable detail over generic assurance. For example, report the changed
behavior and the check that passed instead of saying only that the work is done.
When asking for a decision, name the unresolved choice, its practical impact, and
the information needed from the user.

Do not rewrite verbatim source text, raw command output, required schemas, or code
identifiers merely to satisfy this rule. Summarize or explain them separately when
that helps the reader.

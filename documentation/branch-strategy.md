# Branch strategy and naming

## Workflow
Every major.minor has it's own main branch named _feature/major.minor/main_.
Start by creating a feature branch from that main branch and naming it _feature/major.minor/some-descriptive-name_. 
When ready for review, do a pull request to the _feature/major.minor/main_ branch.

*Note: Keep in mind when starting to develop that merging may be necessary from previous branches.*

## Release
When ready for release, merge from _feature/major.minor/main_ to _release/major.minor_ branch.

## Patch
When making bug fixes on a release use branch _patch/major.minor/main_ as main-branch in the same manor as feature-workflow.
On completion of patch merge to _release/major.minor_

## Brances and version numbers examples
- _feature/1.2/main_ main-branch for 1.2 produces 1.2.0-beta.buildnumber i.e. 1.2.0-beta.4
- _feature/1.2/a-task-to-solve_ feature-branch for 1.2 produces 1.2.0-alpha.nomalized_branchname.buildnumber i.e. 1.2.0-alpha.a_task_to_solve.5
- _patch/1.2/main_ main-patch-branch for 1.2 produces 1.2.patchnumber-beta.buildnumber i.e. 1.2.2-beta.3
- _patch/1.2/a-task-to-solve_ patch-branch for 1.2 produces 1.2.patchnumber-alpha.nomalized_branchname.buildnumber i.e. 1.2.3-alpha.a_task_to_solve.7
- _release/1.2_ for 1.2 produces 1.2.patchnumber i.e. 1.2.4


# branch strategy and naming

## workflow
Every major.minor has it's own main branch named feature/major.minor/main.

Start by creating a feature branch from that main branch and naming it feature/major.minor/some-descriptive-name. 

When ready for review, do a pull request to the feature/major.minor/main branch.

When ready for release, merge from feature/major.minor/main to release/major.minor branch.

*Note: Keep in mind when starting to develop that merging may be necessary from previous branches.*

## examples
- feature/1.2/main main-branch for 1.2 produces 1.2.patchnumber-beta.buildnumber i.e. 1.2.4-beta.5
- feature/1.2/a-task-to-solve for 1.2 produces 1.2.patchnumber-alpha.nomalized_branchname.buildnumber i.e. 1.2.4-alpha.nomalized_branchname.5
- release/1.2 for 1.2 produces 1.2.patchnumber i.e. 1.2.4

## Question
**Q**: why not use patch in alpha and beta too?

**A**: for rpm packages, patch is a part of version and when using buildnumber as patch (may be from different branch builds) there is no way to keep record on which version has higer precedence so we use buildnumber in rpm release tag instead for alpha and beta.
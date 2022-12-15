name: Feature request
about: Suggest an idea for this project
title: Feature Request
labels: ''
assignees: ''
body:
  - type: markdown
    attributes:
      value: |
        Thanks for taking the time to fill out this feature request!
  - type: textarea
    id: problem
    attributes:
      label: What is your problem?
      description: Tell us what problem you want to solve.
    validations:
      required: true
  - type: textarea
    id: reproduce
    attributes:
      label: How to reproduce it
      description: Tell us how to reproduce the problem you want to solve.
      placeholder: Tell us what you see!
    validations:
      required: false


program ftest_person

  use DLite
  use Person

  implicit none

  TPerson      :: person
  DLiteStorage :: storage

  person%read("json",
              "persons.json",
              "d473aa6f-2da3-4889-a88d-0c96186c3fa2")

  person%age = 34

  person%write("json",
               "persons.json")

end program FTest
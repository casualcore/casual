<template>
    <div>
        <!--form>
            <div class="form-group row">
                <label for="keywordInput" class="col-sm-2 col-form-label"><b>Search</b></label>
                <div class="col-sm-6">
                    <input class="form-control" id="keywordInput" type="text" ng-model="searchKeyword">
                </div>
            </div>
        </form-->
        <table class="table-style">
            <thead>
            <tr>
                <th scope="">Name</th>
                <th scope="">Note</th>
                <th scope="">Members</th>
            </tr>
            </thead>
            <tbody>

            <tr v-for="group in groups" :key="group.name">
                <td>
                    <router-link :to="{name: 'group', query: {name: group.name}}">
                        <a class="particle-name-link">{{group.name}}</a>
                    </router-link>

                </td>
                <td>{{group.note}}</td>
                <td>
                   {{group.members.servers.length + group.members.executables.length}}
                </td>

            </tr>

            </tbody>
        </table>

    </div>

</template>

<script>
    export default {
        name: "AllGroupsTable",
        props: ['groups'],
        data() {
            return {
                all: this.groups,
            }
        },
        computed: {
            alls: function () {
                return "";
            }
        }
    }

</script>

